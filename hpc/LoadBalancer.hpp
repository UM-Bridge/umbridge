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

std::string readLineFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    std::string line = "";

    if (file.is_open())
    {
        std::getline(file, line);
    }
    else
    {
        std::cerr << "Unable to open file: " << filename << std::endl;
    }

    return line;
}

using SafeUniqueModelPointer = std::unique_ptr<umbridge::Model, std::function<void (umbridge::Model*)>>;

class JobManager
{
public:
    // Grant exclusive ownership of a model (with a given name) to a caller.
    // The returned object MUST release any resources that it holds once it goes out of scope in the code of the caller.
    // This can be achieved by returning a unique pointer with an appropriate deleter.
    // This method may return a nullptr to deny a request.
    virtual SafeUniqueModelPointer requestModelAccess(const std::string& model_name) = 0;

    // To initialize the load balancer we first need a list of model names that are available on a server.
    // Typically, this can be achieved by simply running the model code and requesting the model names from the server.
    // Therefore, the implementation can most likely use the same mechanism that is also used for granting model access.
    virtual std::vector<std::string> getModelNames() = 0;

    virtual ~JobManager() {};
};

class FileBasedJobManager : public JobManager
{
public:
    virtual SafeUniqueModelPointer requestModelAccess(const std::string& model_name) override
    {
        std::string job_id = submitJob();
        std::string server_url = readURL(job_id);

        SafeUniqueModelPointer client(new umbridge::HTTPModel(server_url, model_name), createModelDeleter(job_id));
        return client;
    }
protected:
    virtual std::string getSubmissionCommand() = 0;
    virtual std::string getCancelationCommand(const std::string& job_id) = 0;

    std::function<void (umbridge::Model*)> createModelDeleter(const std::string& job_id) 
    {
        std::string file_to_delete = getURLFileName(job_id);
        std::string cancelation_command = getCancelationCommand(job_id);
        return [file_to_delete, cancelation_command](umbridge::Model* model) {
            delete model;
            std::filesystem::remove(file_to_delete);
            std::system(cancelation_command.c_str());
        };
    }

    std::string getURLFileName(const std::string& job_id)
    {
        std::filesystem::path url_file_name(url_file_prefix + job_id + url_file_suffix);
        return (url_dir / url_file_name).string();
    }

    std::string readURL(const std::string& job_id)
    {
        return readLineFromFile(getURLFileName(job_id));
    }

    std::string submitJob()
    {
        // Add optional delay to job submissions to prevent issues in some cases.
        if (submission_delay_ms) {
            std::lock_guard<std::mutex> lock(submission_mutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(submission_delay_ms));
        }
        // Submit job and increase job count
        std::string command_output = getCommandOutput(getSubmissionCommand());
        job_count++;

        // Extract the actual job id from the command output
        return parseJobID(command_output);
    }

    virtual std::string parseJobID(const std::string& unparsed_job_id) {
        return unparsed_job_id;
    }

    std::string selectJobScript(const std::string& model_name, bool force_default_submission_script = false)
    {
        namespace fs = std::filesystem;

        const fs::path submission_script_model_specific(
            submission_script_model_specific_prefix + model_name + submission_script_model_specific_suffix);
        std::string job_script = "";

        // Use model specific job script if available, default otherwise.
        if (fs::exists(submission_script_dir / submission_script_model_specific) && !force_default_submission_script)
        {
            std::string job_script = (submission_script_dir / submission_script_model_specific).string();
        }
        else if (fs::exists(submission_script_dir / submission_script_default)) 
        {
            std::string job_script = (submission_script_dir / submission_script_default).string();
        }
        else
        {
            const std::string error_msg = "Job submission script not found: Check that file '" 
                + (submission_script_dir / submission_script_default).string() + "' exists.";
            throw std::runtime_error(error_msg);
        }
        return job_script;
    }
    
    std::filesystem::path submission_script_dir;
    std::filesystem::path submission_script_default;
    // Model-specifc job-script format: <prefix><model_name><suffix>
    std::string submission_script_model_specific_prefix;
    std::string submission_script_model_specific_suffix;

    // URL file format: <prefix><job-id><suffix>
    std::filesystem::path url_dir;
    std::string url_file_prefix;
    std::string url_file_suffix;

    int submission_delay_ms = 0;
    std::mutex submission_mutex;
    
    std::atomic<int32_t> job_count = 0;
};

class HyperQueueJob : public umbridge::Model
{
public:
    static std::atomic<int32_t> job_count;
    HyperQueueJob(std::string model_name, bool start_client=true, 
                                          bool force_default_submission_script=false)
    : Model(model_name)
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
    LoadBalancer(std::string name, std::shared_ptr<JobManager> job_manager) 
    : umbridge::Model(name), job_manager(job_manager) {}

    std::vector<std::size_t> GetInputSizes(const json &config_json = json::parse("{}")) const override
    {
        auto model = job_manager->requestModelAccess(name);
        return model->GetInputSizes(config_json);
    }

    std::vector<std::size_t> GetOutputSizes(const json &config_json = json::parse("{}")) const override
    {
        auto model = job_manager->requestModelAccess(name);
        return model->GetOutputSizes(config_json);
    }

    std::vector<std::vector<double>> Evaluate(const std::vector<std::vector<double>> &inputs, json config_json = json::parse("{}")) override
    {
        auto model = job_manager->requestModelAccess(name);
        return model->Evaluate(inputs, config_json);
    }

    std::vector<double> Gradient(unsigned int outWrt,
                                 unsigned int inWrt,
                                 const std::vector<std::vector<double>> &inputs,
                                 const std::vector<double> &sens,
                                 json config_json = json::parse("{}")) override
    {
        auto model = job_manager->requestModelAccess(name);
        return model->Gradient(outWrt, inWrt, inputs, sens, config_json);
    }

    std::vector<double> ApplyJacobian(unsigned int outWrt,
                                      unsigned int inWrt,
                                      const std::vector<std::vector<double>> &inputs,
                                      const std::vector<double> &vec,
                                      json config_json = json::parse("{}")) override
    {
        auto model = job_manager->requestModelAccess(name);
        return model->ApplyJacobian(outWrt, inWrt, inputs, vec, config_json);
    }

    std::vector<double> ApplyHessian(unsigned int outWrt,
                                     unsigned int inWrt1,
                                     unsigned int inWrt2,
                                     const std::vector<std::vector<double>> &inputs,
                                     const std::vector<double> &sens,
                                     const std::vector<double> &vec,
                                     json config_json = json::parse("{}"))
    {
        auto model = job_manager->requestModelAccess(name);
        return model->ApplyHessian(outWrt, inWrt1, inWrt2, inputs, sens, vec, config_json);
    }

    bool SupportsEvaluate() override
    {
        auto model = job_manager->requestModelAccess(name);
        return model->SupportsEvaluate();
    }
    bool SupportsGradient() override
    {
        auto model = job_manager->requestModelAccess(name);
        return model->SupportsGradient();
    }
    bool SupportsApplyJacobian() override
    {
        auto model = job_manager->requestModelAccess(name);
        return model->SupportsApplyJacobian();
    }
    bool SupportsApplyHessian() override
    {
        auto model = job_manager->requestModelAccess(name);
        return model->SupportsApplyHessian();
    }

private:
    std::shared_ptr<JobManager> job_manager;
};
