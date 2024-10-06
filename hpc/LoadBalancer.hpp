#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <tuple>
#include <memory>
#include <map>
#include <filesystem>
#include "../lib/umbridge.h"

// run and get the result of command
std::string get_command_output(const std::string& command)
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
bool wait_for_file(const std::filesystem::path& file_path, std::chrono::milliseconds polling_cycle)
{
    // Check if the file exists
    while (!std::filesystem::exists(file_path)) {
        // If the file doesn't exist, wait for a certain period
        std::this_thread::sleep_for(polling_cycle);
    }

    return true;
}

std::string read_line_from_file(const std::filesystem::path& file_path)
{
    std::ifstream file(file_path);
    std::string line = "";

    if (file.is_open())
    {
        std::getline(file, line);
    }
    else
    {
        std::cerr << "Unable to open file: " << file_path.string() << std::endl;
    }

    return line;
}

struct Command
{
    std::string exec;
    std::vector<std::string> options;
    std::string target;

    void addOption(const std::string& option)
    {
        options.push_back(option);
    }

    std::string toString() const
    {
        std::string result = exec;
        for (const std::string& s : options)
        {
            result += " " + s;
        }
        result += " " + target;

        return result;
    }
};

void remove_trailing_newline(std::string& s)
{
    if (!s.empty() && s.back() == '\n')
    {
        s.pop_back();
    }
}

// A Job instance escaping its scope would cause the destructor to prematurely cancel the system resource allocation.
// Therefore, copy/move-constructor/assignment are marked as deleted.
// Instead, use explicit ownership mechanisms like std::unique_ptr.
class Job
{
public:
    Job() = default;
    Job(Job& other) = delete;
    Job(Job&& other) = delete;
    Job& operator=(Job& other) = delete;
    Job& operator=(Job&& other) = delete;
    virtual ~Job() = default;

    virtual std::string getJobId() const = 0;
};
// Environment vars: --env KEY1=VAL1 --env KEY2=VAL2
class HyperQueueJob : public Job
{
public:
    explicit HyperQueueJob(const std::vector<std::string>& options, const std::string& target)
    {
        Command command {"./hq", options, target};

        // Makes HQ output "<job id>\n"
        command.addOption("--output-mode=quiet");
        id = get_command_output(command.toString());

        remove_trailing_newline(id);
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
// Environment vars: --export=KEY1=VAL1,KEY2=VAL2
class SlurmJob : public Job
{
public:
    explicit SlurmJob(const std::vector<std::string>& options, const std::string& target)
    {
        Command command {"sbatch", options, target};

        // Makes SLURM output "<job id>[;<cluster name>]\n"
        command.addOption("--parsable");
        std::string output = get_command_output(command.toString());

        id = output.substr(0, output.find(';'));
        remove_trailing_newline(id);
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

class JobSubmitter
{
public:
    virtual ~JobSubmitter() = default;

    virtual std::unique_ptr<Job> submit(const std::string& job_script, const std::map<std::string, std::string>& env) = 0;
};

class HyperQueueSubmitter : public JobSubmitter
{
public:
    HyperQueueSubmitter(std::chrono::milliseconds submission_delay) 
    : submission_delay(submission_delay) {}

    std::unique_ptr<Job> submit(const std::string& job_script, const std::map<std::string, std::string>& env) override 
    {
        // Add optional delay to job submissions to prevent issues in some cases.
        if (submission_delay > std::chrono::milliseconds::zero()) {
            std::lock_guard lock(submission_mutex);
            std::this_thread::sleep_for(submission_delay);
        }

        // Submit job and increase job count
        std::vector<std::string> options = env_to_options(env);
        options.emplace_back("--priority=-" + job_count);
        std::unique_ptr<Job> job = std::make_unique<HyperQueueJob>(options, job_script);
        job_count++;
        return job;
    }
private:
    std::vector<std::string> env_to_options(const std::map<std::string, std::string>& env) const
    {
        std::vector<std::string> options;
        options.reserve(env.size());

        for (const auto& [key, val] : env)
        {
            options.push_back("--env " + key + "=" + val);
        }
        return options;
    }

    std::chrono::milliseconds submission_delay = std::chrono::milliseconds::zero();
    std::mutex submission_mutex;

    std::atomic<int32_t> job_count = 0;
};

class SlurmSubmitter : public JobSubmitter
{

};

class JobCommunicator
{
public:
    virtual ~JobCommunicator() = default;

    virtual std::map<std::string, std::string> getInitMessage() = 0;

    virtual std::string getModelUrl(const std::string& job_id) = 0;
};

class JobCommunicatorFactory
{
public:
    virtual ~JobCommunicatorFactory() = default;

    virtual std::unique_ptr<JobCommunicator> create() = 0;
};

class FilesystemCommunicator : public JobCommunicator
{
public:
    FilesystemCommunicator(std::filesystem::path file_dir, std::chrono::milliseconds polling_cycle) 
    : file_dir(file_dir), polling_cycle(polling_cycle) {}

    ~FilesystemCommunicator() override
    {
        if(!file_path.empty())
        {
            std::filesystem::remove(file_path);
        }
    }

    std::map<std::string, std::string> getInitMessage() override
    {
        std::map<std::string, std::string> msg {{"UMBRIDGE_LOADBALANCER_COMM_FILEDIR", file_dir.string()}};
        return msg;
    }

    std::string getModelUrl(const std::string& job_id) override
    {
        file_path = file_dir / getUrlFileName(job_id);

        std::cout << "Waiting for URL file: " << file_path.string() << std::endl;
        wait_for_file(file_path, polling_cycle);

        // TODO: What if opening the file fails?
        std::string url = read_line_from_file(file_path);
        return url;
    }

private:
    std::string getUrlFileName(const std::string& job_id) const
    {
        return "url-" + job_id + ".txt";
    }

    std::filesystem::path file_dir;
    std::filesystem::path file_path;

    std::chrono::milliseconds polling_cycle;
};

class FilesystemCommunicatorFactory : public JobCommunicatorFactory
{
public:
    FilesystemCommunicatorFactory(std::filesystem::path file_dir, std::chrono::milliseconds polling_cycle)
    : file_dir(file_dir), polling_cycle(polling_cycle)
    {
        std::filesystem::create_directory(file_dir);
    }
    std::unique_ptr<JobCommunicator> create() override
    {
        return std::make_unique<FilesystemCommunicator>(file_dir, polling_cycle);
    }

private:
    std::filesystem::path file_dir;

    std::chrono::milliseconds polling_cycle;
};


struct JobScriptLocator
{
    std::filesystem::path selectJobScript(const std::string& model_name)
    {
        std::filesystem::path script_default = script_dir / script_default_name;
        std::filesystem::path script_model_specific = script_dir / (model_prefix + model_name + model_suffix);

        // Use model specific job script if available, default otherwise.
        if (std::filesystem::exists(script_model_specific))
        {
            return script_model_specific;
        }
        else if (std::filesystem::exists(script_default) )
        {
            return script_default;
        }
        else
        {
            std::string error_msg = "Job script not found: Check that file '" + script_default.string() + "' exists.";
            throw std::runtime_error(error_msg);
        }
    }

    std::filesystem::path getDefaultJobScript()
    {
        return script_dir / script_default_name;
    }

    void printModelJobScripts(std::vector<std::string> model_names) {
        const std::string section_start_delimiter = "==============================MODEL INFO==============================";
        const std::string section_end_delimiter   = "======================================================================";
        
        // Sort the model names in alphabetical order for cleaner output.
        std::sort(model_names.begin(), model_names.end());

        std::cout << section_start_delimiter << std::endl;

        std::cout << "Available models and corresponding job-scripts:\n";
        for (const std::string& model_name : model_names) {
            std::filesystem::path used_job_script = selectJobScript(model_name);
            std::cout << "* Model '" << model_name << "' --> '" << used_job_script << std::endl;
        }
        std::cout << std::endl;

        std::cout << section_end_delimiter << std::endl;
    }


    std::filesystem::path script_dir;

    std::string script_default_name;

    // Model-specific job-script format: <prefix><model_name><suffix>
    std::string model_prefix;
    std::string model_suffix;
};

class JobManager
{
public:
    virtual ~JobManager() = default;

    // Grant exclusive ownership of a model (with a given name) to a caller.
    // The returned object MUST release any resources that it holds once it goes out of scope in the code of the caller.
    // This can be achieved by returning a unique pointer with an appropriate deleter.
    // This method may return a nullptr to deny a request.
    virtual std::unique_ptr<umbridge::Model> requestModelAccess(const std::string& model_name) = 0;

    // To initialize the load balancer we first need a list of model names that are available on a server.
    // Typically, this can be achieved by simply running the model code and requesting the model names from the server.
    // Therefore, the implementation can most likely use the same mechanism that is also used for granting model access.
    virtual std::vector<std::string> getModelNames() = 0;
};


// TODO: Ugly repetition, maybe there is a better way to wrap a job and a model?
class JobModel : public umbridge::Model
{
public:
    JobModel(std::unique_ptr<Job> job, std::unique_ptr<umbridge::Model> model)
    : umbridge::Model(model->GetName()), job(std::move(job)), model(std::move(model)) {}

    std::vector<std::size_t> GetInputSizes(const json &config_json = json::parse("{}")) const override
    {
        return model->GetInputSizes(config_json);
    }

    std::vector<std::size_t> GetOutputSizes(const json &config_json = json::parse("{}")) const override
    {
        return model->GetOutputSizes(config_json);
    }

    std::vector<std::vector<double>> Evaluate(const std::vector<std::vector<double>> &inputs, json config_json = json::parse("{}")) override
    {
        return model->Evaluate(inputs, config_json);
    }

    std::vector<double> Gradient(unsigned int outWrt,
                                 unsigned int inWrt,
                                 const std::vector<std::vector<double>> &inputs,
                                 const std::vector<double> &sens,
                                 json config_json = json::parse("{}")) override
    {
        return model->Gradient(outWrt, inWrt, inputs, sens, config_json);
    }

    std::vector<double> ApplyJacobian(unsigned int outWrt,
                                      unsigned int inWrt,
                                      const std::vector<std::vector<double>> &inputs,
                                      const std::vector<double> &vec,
                                      json config_json = json::parse("{}")) override
    {
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
        return model->ApplyHessian(outWrt, inWrt1, inWrt2, inputs, sens, vec, config_json);
    }

    bool SupportsEvaluate() override
    {
        return model->SupportsEvaluate();
    }
    bool SupportsGradient() override
    {
        return model->SupportsGradient();
    }
    bool SupportsApplyJacobian() override
    {
        return model->SupportsApplyJacobian();
    }
    bool SupportsApplyHessian() override
    {
        return model->SupportsApplyHessian();
    }

private:
    std::unique_ptr<Job> job;
    std::unique_ptr<umbridge::Model> model;
};

// Basic idea:
// 1. Run some command to request a resource allocation on the HPC cluster.
// 2. Launch a model server in the resource allocation.
// 3. Retrieve the URL of the model server.
// 4. Connect to the model server using the URL.
class CommandJobManager : public JobManager
{
public:
    CommandJobManager(
        std::unique_ptr<JobSubmitter> job_submitter, 
        std::unique_ptr<JobCommunicatorFactory> job_comm_factory,
        JobScriptLocator locator) 
        : job_submitter(std::move(job_submitter)), job_comm_factory(std::move(job_comm_factory)), locator(std::move(locator)) {}

    std::unique_ptr<umbridge::Model> requestModelAccess(const std::string& model_name) override
    {
        std::filesystem::path job_script = locator.selectJobScript(model_name);
        std::unique_ptr<JobCommunicator> comm = job_comm_factory->create();
        std::unique_ptr<Job> job = job_submitter->submit(job_script, comm->getInitMessage());
        std::string url = comm->getModelUrl(job->getJobId());
        auto model = std::make_unique<umbridge::HTTPModel>(url, model_name);
        return std::make_unique<JobModel>(std::move(job), std::move(model));
    }

    std::vector<std::string> getModelNames() override 
    {
        std::filesystem::path job_script = locator.getDefaultJobScript();
        std::unique_ptr<JobCommunicator> comm = job_comm_factory->create();
        std::unique_ptr<Job> job = job_submitter->submit(job_script, comm->getInitMessage());
        std::string url = comm->getModelUrl(job->getJobId());
        return umbridge::SupportedModels(url);
    }
    
private:
    std::unique_ptr<JobSubmitter> job_submitter;
    std::unique_ptr<JobCommunicatorFactory> job_comm_factory;
    JobScriptLocator locator;
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
