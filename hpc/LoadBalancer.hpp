#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <tuple>
#include <memory>
#include <filesystem>
#include "../lib/umbridge.h"

// run and get the result of command
std::string getCommandOutput(const std::string command)
{
    FILE *pipe = popen(command.c_str(), "r"); // execute the command and return the output as stream
    if (!pipe)
    {
        std::cerr << "Failed to execute the command: " + command << std::endl;
        return "";
    }

    char buffer[128];
    std::string output;
    while (fgets(buffer, 128, pipe))
    {
        output += buffer;
    }
    pclose(pipe);

    return output;
}

// wait until file is created
bool waitForFile(const std::string &filename)
{
    // Check if the file exists
    while (!std::filesystem::exists(filename)) {
        // If the file doesn't exist, wait for a certain period
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return true;
}

std::string readUrl(const std::string &filename)
{
    std::ifstream file(filename);
    std::string url;
    if (file.is_open())
    {
        std::string file_contents((std::istreambuf_iterator<char>(file)),
                                  (std::istreambuf_iterator<char>()));
        url = file_contents;
        file.close();
    }
    else
    {
        std::cerr << "Unable to open file " << filename << " ." << std::endl;
    }

    // delete the line break
    if (!url.empty())
        url.pop_back();

    return url;
}

std::mutex job_submission_mutex;
int hq_submit_delay_ms = 0;

class HyperQueueJob
{
public:
    static std::atomic<int32_t> job_count;
    HyperQueueJob(std::string model_name, bool start_client=true, 
                                          bool force_default_submission_script=false)
    {
        job_id = submitHQJob(model_name, force_default_submission_script);

        // Get the server URL
        server_url = readUrl("./urls/url-" + job_id + ".txt");

        // Start a client, using unique pointer
        if(start_client)
        {
            client_ptr = std::make_unique<umbridge::HTTPModel>(server_url, model_name);
        }
    }

    ~HyperQueueJob()
    {
        // Cancel the SLURM job
        std::system(("./hq job cancel " + job_id).c_str());

        // Delete the url text file
        std::system(("rm ./urls/url-" + job_id + ".txt").c_str());
    }

    std::string server_url;
    std::unique_ptr<umbridge::HTTPModel> client_ptr;

private:
    std::string submitHQJob(const std::string &model_name, bool force_default_submission_script=false)
    {
        // Add optional delay to job submissions to prevent issues in some cases.
        if (hq_submit_delay_ms) {
            std::lock_guard<std::mutex> lock(job_submission_mutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(hq_submit_delay_ms));
        }
        
        // Use model specific job script if available, default otherwise.
        const std::filesystem::path submission_script_dir("./hq_scripts");
        const std::filesystem::path submission_script_generic("job.sh");
        const std::filesystem::path submission_script_model_specific("job_" + model_name + ".sh");

        std::string hq_command = "./hq submit --output-mode=quiet ";
        hq_command += "--priority=" + std::to_string(job_count) + " ";
        if (std::filesystem::exists(submission_script_dir / submission_script_model_specific) && !force_default_submission_script)
        {
            hq_command += (submission_script_dir / submission_script_model_specific).string();
        }
        else if (std::filesystem::exists(submission_script_dir / submission_script_generic)) 
        {
            hq_command += (submission_script_dir / submission_script_generic).string();
        }
        else
        {
            throw std::runtime_error("Job submission script not found: Check that file 'hq_script/job.sh' exists.");
        }

        // Submit the HQ job and retrieve the HQ job ID.
        std::string job_id = getCommandOutput(hq_command);
        job_count--;

        // Delete the line break.
        if (!job_id.empty())
        {
            job_id.pop_back();
        }

        std::cout << "Waiting for job " << job_id << " to start." << std::endl;

        // Wait for the HQ Job to start
        waitForHQJobState(job_id, "RUNNING");

        // Also wait until job is running and url file is written
        waitForFile("./urls/url-" + job_id + ".txt");

        std::cout << "Job " << job_id << " started." << std::endl;

        return job_id;
    }

    // state = ["WAITING", "RUNNING", "FINISHED", "CANCELED"]
    bool waitForHQJobState(const std::string &job_id, const std::string &state)
    {
        const std::string command = "./hq job info " + job_id + " | grep State | awk '{print $4}'";
        // std::cout << "Checking runtime: " << command << std::endl;
        std::string job_status;

        do
        {
            job_status = getCommandOutput(command);

            // Delete the line break
            if (!job_status.empty())
                job_status.pop_back();

            // Don't wait if there is an error or the job is ended
            if (job_status == "" || (state != "FINISHED" && job_status == "FINISHED") || job_status == "FAILED" || job_status == "CANCELED")
            {
                std::cerr << "Wait for job status failure, status : " << job_status << std::endl;
                return false;
            }

            sleep(1);
        } while (job_status != state);

        return true;
    }

    std::string job_id;
};


class LoadBalancer : public umbridge::Model
{
public:
    LoadBalancer(std::string name) : umbridge::Model(name) {}

    std::vector<std::size_t> GetInputSizes(const json &config_json = json::parse("{}")) const override
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->GetInputSizes(config_json);
    }

    std::vector<std::size_t> GetOutputSizes(const json &config_json = json::parse("{}")) const override
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->GetOutputSizes(config_json);
    }

    std::vector<std::vector<double>> Evaluate(const std::vector<std::vector<double>> &inputs, json config_json = json::parse("{}")) override
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->Evaluate(inputs, config_json);
    }

    std::vector<double> Gradient(unsigned int outWrt,
                                 unsigned int inWrt,
                                 const std::vector<std::vector<double>> &inputs,
                                 const std::vector<double> &sens,
                                 json config_json = json::parse("{}")) override
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->Gradient(outWrt, inWrt, inputs, sens, config_json);
    }

    std::vector<double> ApplyJacobian(unsigned int outWrt,
                                      unsigned int inWrt,
                                      const std::vector<std::vector<double>> &inputs,
                                      const std::vector<double> &vec,
                                      json config_json = json::parse("{}")) override
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->ApplyJacobian(outWrt, inWrt, inputs, vec, config_json);
    }

    std::vector<double> ApplyHessian(unsigned int outWrt,
                                     unsigned int inWrt1,
                                     unsigned int inWrt2,
                                     const std::vector<std::vector<double>> &inputs,
                                     const std::vector<double> &sens,
                                     const std::vector<double> &vec,
                                     json config_json = json::parse("{}"))
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->ApplyHessian(outWrt, inWrt1, inWrt2, inputs, sens, vec, config_json);
    }

    bool SupportsEvaluate() override
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->SupportsEvaluate();
    }
    bool SupportsGradient() override
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->SupportsGradient();
    }
    bool SupportsApplyJacobian() override
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->SupportsApplyJacobian();
    }
    bool SupportsApplyHessian() override
    {
        HyperQueueJob hq_job(name);
        return hq_job.client_ptr->SupportsApplyHessian();
    }
};
