#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <tuple>
#include <memory>
#include "lib/umbridge.h"

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

// state = ["PENDING","RUNNING","COMPLETED","FAILED","CANCELLED"]
bool waitForJobState(const std::string &job_id, const std::string &state = "COMPLETED")
{
    const std::string command = "scontrol show job " + job_id + " | grep -oP '(?<=JobState=)[^ ]+'";
    // std::cout << "Checking runtime: " << command << std::endl;
    std::string job_status;

    do
    {
        job_status = getCommandOutput(command);

        // Delete the line break
        if (!job_status.empty())
            job_status.pop_back();

        // Don't wait if there is an error or the job is ended
        if (job_status == "" || (state != "COMPLETE" && job_status == "COMPLETED") || job_status == "FAILED" || job_status == "CANCELLED")
        {
            std::cerr << "Wait for job status failure, status : " << job_status << std::endl;
            return false;
        }
        // std::cout<<"Job status: "<<job_status<<std::endl;
        sleep(1);
    } while (job_status != state);

    return true;
}

// Check for every 100 ms, wait for maximum 20 second
bool waitForFile(const std::string &filename, int time_out = 20)
{
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(time_out); // wait for maximum 10 seconds

    const std::string command = "while [ ! -f " + filename + " ]; do sleep 0.1; done";
    // std::cout << "Waiting for file: " << command << std::endl;
    std::system(command.c_str());
    auto end_time = std::chrono::steady_clock::now();

    if (end_time - start_time > timeout)
    {
        std::cerr << "Timeout reached waiting for file " << filename << std::endl;
        return false;
    }

    return true;
}

// Start a slurm job and return job id
std::string submitJob(const std::string &command)
{
    std::string sbatch_command = command + " | awk '{print $4}'"; // extract job ID from sbatch output
    std::cout << "Submitting job with command: " << command << std::endl;

    std::string job_id;
    int i = 0;
    do
    {
        job_id = getCommandOutput(sbatch_command);

        // Delete the line break
        if (!job_id.empty())
            job_id.pop_back();

        std::cout << "job_id: " << job_id << std::endl;
        ++i;

    } while (waitForJobState(job_id, "RUNNING") == false && i < 3 && waitForFile("./urls/url-" + job_id + ".txt", 20) == false);
    // Wait to start all nodes on the cluster, call scontrol for every 1 sceond to check
    // Also wait until job is running and url file is written
    // Try maximum 3 times

    // Check if the job is running
    if (waitForJobState(job_id, "RUNNING") == false || waitForFile("./urls/url-" + job_id + ".txt", 10) == false)
    {
        std::cout << "Submit job failure." << std::endl;
        exit(-1);
    }

    return job_id;
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

class SingleSlurmJob
{
public:
    SingleSlurmJob(std::string model_name = "forward")
    {
        // start a SLURM job for single request
        job_id = submitJob("sbatch model.slurm");

        const std::string server_url = readUrl("./urls/url-" + job_id + ".txt"); // read server url from txt file
        // May use $SLURM_LOCALID in a .slurm file later

        std::cout << "Hosting sub-server at : " << server_url << std::endl;

        // List supported models
        std::vector<std::string> models = umbridge::SupportedModels(server_url);
        std::cout << "Supported models: " << std::endl;
        for (auto model : models)
        {
            std::cout << "  " << model << std::endl;
        }
        std::cout << "Using model: " << model_name << std::endl;

        // Start a client, using unique pointer
        client_ptr = std::make_unique<umbridge::HTTPModel>(server_url, model_name); // use the first model avaliable on server by default
    }

    ~SingleSlurmJob()
    {
        // Cancel the SLURM job
        std::system(("scancel " + job_id).c_str());

        // Delete the url text file
        std::system(("rm ./urls/url-" + job_id + ".txt").c_str());
    }

    std::unique_ptr<umbridge::HTTPModel> client_ptr;

private:
    std::string job_id;
};

// state = ["WAITING", "RUNNING", "FINISHED", "CANCELED"]
bool waitForHQJobState(const std::string &job_id, const std::string &state = "COMPLETED")
{
    const std::string command = "hq job info " + job_id + " | grep State | awk '{print $4}'";
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
        // std::cout<<"Job status: "<<job_status<<std::endl;
        sleep(1);
    } while (job_status != state);

    return true;
}

std::string submitHQJob()
{
    std::string hq_command = "hq submit --output-mode=quiet hq_scripts/job.sh";

    std::string job_id;
    int i = 0;
    do
    {
        job_id = getCommandOutput(hq_command);

        // Delete the line break
        if (!job_id.empty())
            job_id.pop_back();

        ++i;
    } while (waitForHQJobState(job_id, "RUNNING") == false && i < 3 && waitForFile("./urls/hqjob-" + job_id + ".txt", 10) == false);
    // Wait for the HQ Job to start
    // Also wait until job is running and url file is written
    // Try maximum 3 times

    // Check if the job is running
    if (waitForHQJobState(job_id, "RUNNING") == false || waitForFile("./urls/hqjob-" + job_id + ".txt", 10) == false)
    {
        std::cout << "Submit job failure." << std::endl;
        exit(-1);
    }

    return job_id;
}

class HyperQueueJob
{
public:
    HyperQueueJob(std::string model_name = "forward")
    {
        job_id = submitHQJob();

        // Get the slurm job id
        const std::string slurm_id = readUrl("./urls/hqjob-" + job_id + ".txt");

        // Get the server URL
        const std::string server_url = readUrl("./urls/url-" + slurm_id + ".txt");

        // Start a client, using unique pointer
        client_ptr = std::make_unique<umbridge::HTTPModel>(server_url, model_name); // always uses the model "forward"
    }

    ~HyperQueueJob()
    {
        // Cancel the SLURM job
        std::system(("hq job cancel " + job_id).c_str());

        // Delete the url text file
        std::system(("rm ./urls/hqjob-" + job_id + ".txt").c_str());
    }

    std::unique_ptr<umbridge::HTTPModel> client_ptr;

private:
    std::string job_id;
};


class LoadBalancer : public umbridge::Model
{
public:
    LoadBalancer(std::string name = "forward") : umbridge::Model(name)
    {
        // Setup HyperQueue server
        std::system("hq server start &");
        sleep(1); // Workaround: give the HQ server enough time to start.

        // Create allocation queue
        std::system("hq_scripts/allocation_queue.sh");
    }

    std::vector<std::size_t> GetInputSizes(const json &config_json = json::parse("{}")) const override
    {
        HyperQueueJob hq_job;
        return hq_job.client_ptr->GetInputSizes(config_json);
    }

    std::vector<std::size_t> GetOutputSizes(const json &config_json = json::parse("{}")) const override
    {
        HyperQueueJob hq_job;
        return hq_job.client_ptr->GetOutputSizes(config_json);
    }

    std::vector<std::vector<double>> Evaluate(const std::vector<std::vector<double>> &inputs, json config_json = json::parse("{}")) override
    {
        HyperQueueJob hq_job;
        return hq_job.client_ptr->Evaluate(inputs, config_json);
    }

    std::vector<double> Gradient(unsigned int outWrt,
                                 unsigned int inWrt,
                                 const std::vector<std::vector<double>> &inputs,
                                 const std::vector<double> &sens,
                                 json config_json = json::parse("{}")) override
    {
        HyperQueueJob hq_job;
        return hq_job.client_ptr->Gradient(outWrt, inWrt, inputs, sens, config_json);
    }

    std::vector<double> ApplyJacobian(unsigned int outWrt,
                                      unsigned int inWrt,
                                      const std::vector<std::vector<double>> &inputs,
                                      const std::vector<double> &vec,
                                      json config_json = json::parse("{}")) override
    {
        HyperQueueJob hq_job;
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
        HyperQueueJob hq_job;
        return hq_job.client_ptr->ApplyHessian(outWrt, inWrt1, inWrt2, inputs, sens, vec, config_json);
    }

    bool SupportsEvaluate() override
    {
        HyperQueueJob hq_job;
        return hq_job.client_ptr->SupportsEvaluate();
    }
    bool SupportsGradient() override
    {
        HyperQueueJob hq_job;
        return hq_job.client_ptr->SupportsGradient();
    }
    bool SupportsApplyJacobian() override
    {
        HyperQueueJob hq_job;
        return hq_job.client_ptr->SupportsApplyJacobian();
    }
    bool SupportsApplyHessian() override
    {
        HyperQueueJob hq_job;
        return hq_job.client_ptr->SupportsApplyHessian();
    }
};
