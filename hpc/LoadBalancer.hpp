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
std::string getCommandOutput(const std::string& command)
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

class JobManager
{
public:
    // Grant exclusive ownership of a model (with a given name) to a caller.
    // The returned object MUST release any resources that it holds once it goes out of scope in the code of the caller.
    // This can be achieved by returning a unique pointer with an appropriate deleter.
    // This method may return a nullptr to deny a request.
    virtual std::unique_ptr<umbridge::Model> requestModelAccess(const std::string& model_name) = 0;

    // To initialize the load balancer we first need a list of model names that are available on a server.
    // Typically, this can be achieved by simply running the model code and requesting the model names from the server.
    // Therefore, the implementation can most likely use the same mechanism that is also used for granting model access.
    virtual std::vector<std::string> getModelNames() = 0;

    virtual ~JobManager() = default;
};

void remove_trailing_newline(std::string& s)
{
    if (!s.empty() && s.back() == '\n')
    {
        s.pop_back();
    }
}

class Job
{
public:
    Job() = default;
    Job(Job &other) = delete;
    Job(Job &&other) = delete;
    Job &operator=(Job &other) = delete;
    Job &operator=(Job &&other) = delete;
    virtual ~Job() = default;

    virtual std::string getJobId() const = 0;
};

class HyperQueueJob : public Job
{
public:
    explicit HyperQueueJob(const std::string& command)
    {
        std::string output = getCommandOutput(command);
        remove_trailing_newline(output);
        id = output;
    }

    ~HyperQueueJob() override
    {
        std::system(("./hq job cancel " + id).c_str());
    }

    std::string getJobId() const override
    {
        return id;
    }
    
private:
    std::string id;
};

class SlurmJob : public Job
{
public:
    explicit SlurmJob(const std::string& command)
    {
        std::string output = getCommandOutput(command);
        id = output.substr(0, output.find(';'));
    }

    ~SlurmJob() override
    {
        std::system(("scancel " + id).c_str());
    }

    std::string getJobId() const override
    {
        return id;
    }
    
private:
    std::string id;
};

/*
std::string submit_hyperqueue_job(const std::string& command)
{
    std::string id = getCommandOutput(command);
    remove_trailing_newline(id);
    return id;
}

std::string submit_slurm_job(const std::string& command)
{
    std::string id = getCommandOutput(command);
    return id.substr(0, id.find(';'));
}

void cancel_hyperqueue_job(const std::string& id)
{
    std::system(("./hq job cancel " + id).c_str());
}

void cancel_slurm_job(const std::string& id)
{
    std::system(("scancel " + id).c_str());
}

template <typename SubmitFunction, typename CancelFunction>
class Job
{
public:
    Job(std::string submit_command, SubmitFunction submit, CancelFunction cancel) : id(submit(submit_command)), cancel(cancel) {}
    Job(Job &other) = delete;
    Job(Job &&other) = delete;
    Job &operator=(Job &other) = delete;
    Job &operator=(Job &&other) = delete;
    ~Job()
    {
        cancel(id);
    }

private:
    std::string id;
    CancelFunction cancel;
};

class HyperQueueJob : public Job
{
    HyperQueueJob()
};


using SlurmJob = Job<decltype(&submit_slurm_job), decltype(&cancel_slurm_job)>;
using HyperQueueJob = Job<decltype(&submit_hyperqueue_job), decltype(&cancel_hyperqueue_job)>;
*/

// Basic idea:
// 1. Run some command to request a resource allocation on the HPC cluster.
// 2. Launch a model server in the resource allocation.
// 3. Retrieve the URL of the model server.
// 4. Connect to the model server using the URL.
template <typename Job>
class CommandJobManager : public JobManager
{
public:
    std::unique_ptr<umbridge::Model> requestModelAccess(const std::string& model_name) override
    {
        std::string job_id = submitJob();
        std::string server_url = readURL(job_id);

        std::unique_ptr<umbridge::Model> model = std::make_unique<umbridge::HTTPModel>(model_name, server_url);
        return model;
    }
private:
    virtual std::string getSubmissionCommand() = 0;

    std::string getURLFileName(const std::string& job_id) const
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
        if (submission_delay_ms > 0) {
            std::lock_guard lock(submission_mutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(submission_delay_ms));
        }
        // Submit job and increase job count
        Job job(getSubmissionCommand()); // getSubmissionCommand may depend on job_count. Possible race condition!
        job_count++;
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
    // Model-specific job-script format: <prefix><model_name><suffix>
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

class HyperQueueJob
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
                                     json config_json = json::parse("{}")) override
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
