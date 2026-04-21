#include "../lib/umbridge.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <regex>
#include <sstream>

// Run a shell command and get the result.
// Warning: Prone to injection, do not call with user-supplied arguments.
// Note: POSIX specific and may not run on other platforms (e.g. Windows), but most HPC systems are POSIX-compliant.
// Using an external library (e.g. Boost) would be cleaner, but not worth the effort of managing another dependency.
std::string getCommandOutput(const std::string& command) {
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), &pclose);

    if (!pipe) {
        std::string errorMsg = "Failed to run command: " + command + "\n"
                              + "popen failed with error: " + std::strerror(errno) + "\n"; 
        throw std::runtime_error(errorMsg);
    }

    // Buffer size can be small and is largely unimportant since most commands we use only return a single line.
    std::array<char, 128> buffer;
    std::string output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get())) {
        output += buffer.data();
    }

    return output;
}

// Wait until a file exists using polling. 
// Improvements: Use inotify instead to save cpu cycles
void waitForFile(const std::filesystem::path& filePath, std::chrono::milliseconds pollingCycle) {
    while (!std::filesystem::exists(filePath)) {
        std::this_thread::sleep_for(pollingCycle);
    }
}

std::string readLineFromFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::string errorMsg = "Unable to open file: '" + filePath.string() + "'\n";
        throw std::runtime_error(errorMsg);
    }

    std::string line;
    std::getline(file, line);

    return line;
}

void removeTrailingNewline(std::string& s) {
    if (!s.empty() && s.back() == '\n') {
        s.pop_back();
    }
}

struct Command {
    std::string exec;
    std::vector<std::string> options;
    std::string target;

    void addOption(const std::string& option) {
        options.push_back(option);
    }

    std::string toString() const {
        std::string result = exec;
        for (const std::string& s : options)
        {
            result += " " + s;
        }
        result += " " + target;

        return result;
    }
};


// A Job represents a resource allocation on an HPC system and has a unique string ID.
// Note: A Job instance escaping its scope would cause the destructor to prematurely cancel the system resource allocation.
// Therefore, copy/move-constructor/assignment are marked as deleted.
// Instead, use explicit ownership mechanisms like std::unique_ptr.
class Job {
public:
    Job() = default;
    Job(Job& other) = delete;
    Job(Job&& other) = delete;
    Job& operator=(Job& other) = delete;
    Job& operator=(Job&& other) = delete;
    virtual ~Job() = default;

    virtual std::string getJobId() const = 0;
    virtual void setBusyness(bool status) = 0;
    virtual bool getBusyness() = 0;
};


// Submits SLURM job to spawn model server in compute node
// Suggestion: change into job arrays
class SlurmJob : public Job {
public:
    SlurmJob(const std::string id): jobID(id) {}

    void setBusyness(bool status) override {
        isBusy = status;
    }
    
    bool getBusyness() override {
        return isBusy;
    }

    ~SlurmJob() override {
        std::system(("scancel " + jobID).c_str());
    }

    std::string getJobId() const override {
        return jobID;
    }
    
private:
    std::string jobID;
    bool isBusy = false;
};


// Factory class meant to provide a more high-level interface for job submission.
// In particular, makes it possible to pass environment variables to a job as key-value pairs. 
class JobSubmitter {
public:
    virtual ~JobSubmitter() = default;

    virtual std::string submit(int numServer, const std::string& jobScript, const std::map<std::string, std::string>& env) = 0;
};

class SlurmSubmitter : public JobSubmitter {
public:
    SlurmSubmitter(std::chrono::milliseconds submissionDelay) 
    : submissionDelay(submissionDelay) {}

    std::string submit(int numServer, const std::string& jobScript, const std::map<std::string, std::string>& env) override {
        // Add optional delay to job submissions to prevent issues in some cases.
        if (submissionDelay > std::chrono::milliseconds::zero()) {
            std::lock_guard lock(submissionMutex);
            std::this_thread::sleep_for(submissionDelay);
        }

        // Submit job
        std::vector<std::string> options = envToOptions(env);
        Command command {"sbatch", options, jobScript};

        // Makes SLURM output "<job id>[;<cluster name>]\n"
        command.addOption("--parsable");
        command.addOption("--array=1-" + std::to_string(numServer));
        std::string output = getCommandOutput(command.toString());

	    std::regex jobIDRegex(R"(^(\d+)(?:;[a-zA-Z0-9_-]+)?$)");
	    std::istringstream stream(output);
      	std::string line;

        std::string jobID;
        while (std::getline(stream, line)) {
            std::smatch match;
            if (std::regex_match(line, match, jobIDRegex)) {
                jobID = match[1];
            }
        }
        removeTrailingNewline(jobID);
        return jobID;
    }
    
private:
    // SLURM environment variables: --export=KEY1=VAL1,KEY2=VAL2,...
    std::vector<std::string> envToOptions(const std::map<std::string, std::string>& env) const {
        // By default include all SLURM_* and SPANK option environment variables.
        std::string envOption = "--export=ALL";

        for (const auto& [key, val] : env) {
            envOption += "," + key + "=" + val;
        }

        return {envOption};
    }
    
    std::chrono::milliseconds submissionDelay = std::chrono::milliseconds::zero();
    std::mutex submissionMutex;
};


// A JobCommunicator is used to establish communication between the load balancer and a submitted job script.
// The JobCommunicator first generates an initial message of key-value pairs 
// which are then passed to the job script via environment variables.
// This message should allow the job script to send back the URL of the hosted model to the load balancer.
// Note: Like a Job, a JobCommunicator shall not be copied or moved.
class JobCommunicator {
public:
    JobCommunicator() = default;
    JobCommunicator(JobCommunicator& other) = delete;
    JobCommunicator(JobCommunicator&& other) = delete;
    JobCommunicator& operator=(JobCommunicator& other) = delete;
    JobCommunicator& operator=(JobCommunicator&& other) = delete;
    virtual ~JobCommunicator() = default;

    virtual std::map<std::string, std::string> getInitMessage() = 0;

    virtual std::string getModelUrl(const std::string& jobID) = 0;
};

class JobCommunicatorFactory {
public:
    virtual ~JobCommunicatorFactory() = default;

    virtual std::unique_ptr<JobCommunicator> create() = 0;
};

class FilesystemCommunicator : public JobCommunicator {
public:
    FilesystemCommunicator(std::filesystem::path fileDir, std::chrono::milliseconds pollingCycle) 
    : fileDir(std::move(fileDir)), pollingCycle(pollingCycle) {}

    ~FilesystemCommunicator() override {
        if(!filePath.empty()) {
            std::filesystem::remove(filePath);
        }
    }

    // Tell the job script which directory the URL file should be written to.
    std::map<std::string, std::string> getInitMessage() override {
        std::map<std::string, std::string> msg {{"UMBRIDGE_LOADBALANCER_COMM_FILEDIR", fileDir.string()}};
        return msg;
    }

    std::string getModelUrl(const std::string& jobID) override {
        filePath = fileDir / getUrlFileName(jobID);

        std::cout << "Waiting for URL file: " << filePath.string() << std::endl;
        waitForFile(filePath, pollingCycle);

        // TODO: What if opening the file fails?
        std::string url = readLineFromFile(filePath);
        return url;
    }
    
    // Potentially add a is_ready function to check if server is up
    /*
    bool is_ready() {
        try to connect to model via url
    }
    */

private:
    // Currently, the naming of the URL file is hard-code.
    // In the future, it might be better to have the communicator itself generate the filename and then send it to the job script.
    std::string getUrlFileName(const std::string& jobID) const {
        return "url-" + jobID + ".txt";
    }

    std::filesystem::path fileDir;
    std::filesystem::path filePath;

    std::chrono::milliseconds pollingCycle;
};

class FilesystemCommunicatorFactory : public JobCommunicatorFactory {
public:
    FilesystemCommunicatorFactory(std::filesystem::path fileDir, std::chrono::milliseconds pollingCycle)
    : fileDir(fileDir), pollingCycle(pollingCycle) {
        std::filesystem::create_directory(fileDir);
    }

    std::unique_ptr<JobCommunicator> create() override {
        return std::make_unique<FilesystemCommunicator>(fileDir, pollingCycle);
    }

private:
    std::filesystem::path fileDir;
    std::chrono::milliseconds pollingCycle;
};


// A JobScriptLocator specifies where the job script for a particular model is located.
struct JobScriptLocator {
    std::filesystem::path selectJobScript(const std::string& modelName) {
        std::filesystem::path scriptDefault = scriptDir / scriptDefaultName;
        std::filesystem::path scriptModelSpecific = scriptDir / (modelPrefix + modelName + modelSuffix);

        // Use model specific job script if available, default otherwise.
        if (std::filesystem::exists(scriptModelSpecific)) {
            return scriptModelSpecific;
        } else if (std::filesystem::exists(scriptDefault)) {
            return scriptDefault;
        } else {
            std::string errorMsg = "Job script not found: Check that file '" + scriptDefault.string() + "' exists.\n";
            throw std::runtime_error(errorMsg);
        }
    }

    std::filesystem::path getDefaultJobScript() {
        return scriptDir / scriptDefaultName;
    }

    void printModelJobScripts(std::vector<std::string> modelNames) {
        const std::string sectionStartDelimiter = "==============================MODEL INFO==============================";
        const std::string sectionEndDelimiter   = "======================================================================";
        
        // Sort the model names in alphabetical order for cleaner output.
        std::sort(modelNames.begin(), modelNames.end());

        std::cout << sectionStartDelimiter << std::endl;

        std::cout << "Available models and corresponding job-scripts:\n";
        for (const std::string& modelName : modelNames) {
            std::filesystem::path usedJobScript = selectJobScript(modelName);
            std::cout << "* Model '" << modelName << "' --> '" << usedJobScript.string() << "'" << std::endl;
        }
        std::cout << std::endl;

        std::cout << sectionEndDelimiter << std::endl;
    }


    std::filesystem::path scriptDir;

    std::string scriptDefaultName;

    // Model-specific job-script format: <prefix><modelName><suffix>
    std::string modelPrefix;
    std::string modelSuffix;
};


// A Job manager provides access to an UM-Bridge model on an HPC system.
class JobManager {
public:
    virtual ~JobManager() = default;

    // Grant exclusive ownership of a model (with a given name) to a caller.
    virtual std::shared_ptr<umbridge::Model> requestModelAccess(const std::string& modelName) = 0;

    // To initialize the load balancer we first need a list of model names that are available on a server.
    // Typically, this can be achieved by simply running the model code and requesting the model names from the server.
    // Therefore, the implementation can most likely use the same mechanism that is also used for granting model access.
    virtual std::vector<std::string> getModelName(std::string url) = 0;
    
    virtual std::set<std::string> getModelNameArray() = 0;
};


// TODO: Ugly repetition, maybe there is a better way to wrap a job and a model?
class JobModel : public umbridge::Model {
public:
    JobModel(std::unique_ptr<Job> job, std::unique_ptr<umbridge::Model> model)
    : umbridge::Model(model->GetName()), job(std::move(job)), model(std::move(model)) {}

    std::vector<std::size_t> GetInputSizes(const json &config_json = json::parse("{}")) const override {
        auto inputsizes = model->GetInputSizes(config_json);
        job->setBusyness(false);
        return inputsizes;
    }

    std::vector<std::size_t> GetOutputSizes(const json &config_json = json::parse("{}")) const override {
        auto outputSizes = model->GetOutputSizes(config_json);
        job->setBusyness(false);
        return outputSizes;
    }

    std::vector<std::vector<double>> Evaluate(const std::vector<std::vector<double>> &inputs, 
                                              json config_json = json::parse("{}")) override {
        auto output = model->Evaluate(inputs, config_json);
        job->setBusyness(false);
        return output;
    }

    std::vector<double> Gradient(unsigned int outWrt,
                                 unsigned int inWrt,
                                 const std::vector<std::vector<double>> &inputs,
                                 const std::vector<double> &sens,
                                 json config_json = json::parse("{}")) override {
        auto gradient = model->Gradient(outWrt, inWrt, inputs, sens, config_json);
        job->setBusyness(false);
        return gradient;
    }

    std::vector<double> ApplyJacobian(unsigned int outWrt,
                                      unsigned int inWrt,
                                      const std::vector<std::vector<double>> &inputs,
                                      const std::vector<double> &vec,
                                      json config_json = json::parse("{}")) override {
        auto applyJacobian = model->ApplyJacobian(outWrt, inWrt, inputs, vec, config_json);
        job->setBusyness(false);
        return applyJacobian; 
    }

    std::vector<double> ApplyHessian(unsigned int outWrt,
                                     unsigned int inWrt1,
                                     unsigned int inWrt2,
                                     const std::vector<std::vector<double>> &inputs,
                                     const std::vector<double> &sens,
                                     const std::vector<double> &vec,
                                     json config_json = json::parse("{}")) override {
        auto applyHessian = model->ApplyHessian(outWrt, inWrt1, inWrt2, inputs, sens, vec, config_json);
        job->setBusyness(false);
        return applyHessian;
    }

    bool SupportsEvaluate() override {
        auto supportsEvaluate = model->SupportsEvaluate();
        job->setBusyness(false);
        return supportsEvaluate;
    }
    bool SupportsGradient() override {
        auto supportsGradient = model->SupportsGradient();
        job->setBusyness(false);
        return supportsGradient;
    }
    bool SupportsApplyJacobian() override {
        auto supportsJacobian = model->SupportsApplyJacobian();
        job->setBusyness(false);
        return supportsJacobian;
    }
    bool SupportsApplyHessian() override {
        auto supportsHessian = model->SupportsApplyHessian();
        job->setBusyness(false);
        return supportsHessian;
    }
    
    bool job_status() {
        try {
            GetInputSizes(json::parse("{}"));
        }
        catch (std::exception& e) {
            std::cout << "Model server is no longer running" << std::endl;
            return false;
        }
        return true;
    }
    
    Job* getJob() {
        return job.get();
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
class CommandJobManager : public JobManager {
public:
    CommandJobManager(
        std::unique_ptr<JobSubmitter> jobSubmitter, 
        std::unique_ptr<JobCommunicatorFactory> jobCommFactory,
        JobScriptLocator locator,
        int numServer) 
        : jobSubmitter(std::move(jobSubmitter)), jobCommFactory(std::move(jobCommFactory)), locator(std::move(locator)), numServer(numServer) {
            // Submit slurm jobs to start model server
            spawnServers();
        }
        // create spawn servers function to use in constructor and restarts
        // adapt to use job arrays

    std::shared_ptr<umbridge::Model> requestModelAccess(const std::string& modelName) override {
        // Sould select an available model from the vector and return 
        // Suggestion: make a request class that destructs and mark busyness. Maybe a bad idea (many temps)
        // Mutex here for first come first serve
        // Problem: deadlock here when more threads than available servers
        // Cause: Running model crashes but leaves extra thread(s) dangling
        // Solution: Kill all threads when crashes / Mark all threads as completed
        // even better: refactor code to account for crashed/terminated servers
        std::scoped_lock serverLock{serverMutex};
        int iter = 0;
        while (true) {
            if (serverArray.size() == 0) {
                std::cout << "No available servers running." << std::endl; // Need to make it exit properly
                return nullptr;
            }
            for (auto& tmp : serverArray) { // to solve; serverArray may contain duplicate slurm allocation when multiple model names are present in one server
                auto& server = tmp.first;
                if (!server->getJob()->getBusyness()) {
                    server->getJob()->setBusyness(true);
                    return server;
                }
                if (iter == 50) {
                    bool serverStatus = server->job_status();
                    if (tmp.second != serverStatus) {
                        serverArray.erase(tmp.first);
                    }
                }
            }
            iter = (iter == 50) ? 0: iter + 1; // To prevent overflow for long runs
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }    
    }

    void spawnServers() {
        std::filesystem::path jobScript = locator.getDefaultJobScript();
        std::unique_ptr<JobCommunicator> comm = jobCommFactory->create();
        std::string jobID = jobSubmitter->submit(numServer, jobScript, comm->getInitMessage());
        for (int i = 1; i <= numServer; i++) {
            std::string jobArrayID = jobID + "_" + std::to_string(i);
            std::string url = comm->getModelUrl(jobArrayID);
            auto modelName = getModelName(url);
            modelNames.insert(modelName[0]); // Problem: May have multiple names in one server
            auto model = std::make_unique<umbridge::HTTPModel>(url, modelName[0]);
            std::unique_ptr<Job> job = std::make_unique<SlurmJob>(jobArrayID);
            serverArray.insert({std::make_shared<JobModel>(std::move(job), std::move(model)), true});
        }
    }

    std::vector<std::string> getModelName(std::string url) override {
        return umbridge::SupportedModels(url);
    }
    
    std::set<std::string> getModelNameArray() override {
        return modelNames;
    }
private:
    std::mutex serverMutex;
    std::unique_ptr<JobSubmitter> jobSubmitter;
    std::unique_ptr<JobCommunicatorFactory> jobCommFactory;
    JobScriptLocator locator;
    int numServer;
    std::map<std::shared_ptr<JobModel>, bool> serverArray;
    std::set<std::string> modelNames;
};


// A LoadBalancer acts like a regular UM-Bridge model with the key difference, that incoming requests are
// redirected to models running in a job allocation of an HPC system.
class LoadBalancer : public umbridge::Model {
public:
    LoadBalancer(std::string name, std::shared_ptr<JobManager> jobManager) 
    : umbridge::Model(name), jobManager(jobManager) {}

    std::vector<std::size_t> GetInputSizes(const json &config_json = json::parse("{}")) const override {
        auto model = jobManager->requestModelAccess(name);
        return model->GetInputSizes(config_json);
    }

    std::vector<std::size_t> GetOutputSizes(const json &config_json = json::parse("{}")) const override {
        auto model = jobManager->requestModelAccess(name);
        return model->GetOutputSizes(config_json);
    }

    std::vector<std::vector<double>> Evaluate(const std::vector<std::vector<double>> &inputs, 
                                              json config_json = json::parse("{}")) override {
        auto model = jobManager->requestModelAccess(name);
        return model->Evaluate(inputs, config_json);
    }

    std::vector<double> Gradient(unsigned int outWrt,
                                 unsigned int inWrt,
                                 const std::vector<std::vector<double>> &inputs,
                                 const std::vector<double> &sens,
                                 json config_json = json::parse("{}")) override {
        auto model = jobManager->requestModelAccess(name);
        return model->Gradient(outWrt, inWrt, inputs, sens, config_json);
    }

    std::vector<double> ApplyJacobian(unsigned int outWrt,
                                      unsigned int inWrt,
                                      const std::vector<std::vector<double>> &inputs,
                                      const std::vector<double> &vec,
                                      json config_json = json::parse("{}")) override {
        auto model = jobManager->requestModelAccess(name);
        return model->ApplyJacobian(outWrt, inWrt, inputs, vec, config_json);
    }

    std::vector<double> ApplyHessian(unsigned int outWrt,
                                     unsigned int inWrt1,
                                     unsigned int inWrt2,
                                     const std::vector<std::vector<double>> &inputs,
                                     const std::vector<double> &sens,
                                     const std::vector<double> &vec,
                                     json config_json = json::parse("{}")) override {
        auto model = jobManager->requestModelAccess(name);
        return model->ApplyHessian(outWrt, inWrt1, inWrt2, inputs, sens, vec, config_json);
    }

    bool SupportsEvaluate() override {
        auto model = jobManager->requestModelAccess(name);
        return model->SupportsEvaluate();
    }
    bool SupportsGradient() override {
        auto model = jobManager->requestModelAccess(name);
        return model->SupportsGradient();
    }
    bool SupportsApplyJacobian() override {
        auto model = jobManager->requestModelAccess(name);
        return model->SupportsApplyJacobian();
    }
    bool SupportsApplyHessian() override {
        auto model = jobManager->requestModelAccess(name);
        return model->SupportsApplyHessian();
    }

private:
    std::shared_ptr<JobManager> jobManager;
};

