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
#include <condition_variable>
#include <queue>

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
    virtual bool getBusyness() const = 0;
};


// Submits SLURM job to spawn model server in compute node
class SlurmJob : public Job {
public:
    SlurmJob(const std::string id): jobID(id) {}
    
    // Called by the load balancer so the job can notify waiters when it becomes free.
    void setFreeNotifier(std::function<void()> notifier) {
        freeNotifier = std::move(notifier);
    }

    void setBusyness(bool busyness) override {
        isBusy = busyness;
        // Notify the load balancer's request queue that a server has become available.
        if (!isBusy && freeNotifier) {
            freeNotifier();
        }
    }
    
    bool getBusyness() const override {
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
    std::function<void()> freeNotifier;
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

    virtual std::map<std::string, std::string> getInitMessage() const = 0;

    virtual std::string getModelUrl(const std::string& jobID) = 0;
};

class JobCommunicatorFactory {
public:
    virtual ~JobCommunicatorFactory() = default;

    virtual std::unique_ptr<JobCommunicator> create() const = 0;
};

class FilesystemCommunicator : public JobCommunicator {
public:
    FilesystemCommunicator(std::filesystem::path fileDir, std::chrono::milliseconds pollingCycle) 
    : fileDir(std::move(fileDir)), pollingCycle(pollingCycle) {}

    ~FilesystemCommunicator() override {
        for (auto name: urlFilenames) {
            std::string removableUrlFile = fileDir / name;
            if(!removableUrlFile.empty()) {
                std::filesystem::remove(removableUrlFile);
            }
        }
    } 
        
    // Tell the job script which directory the URL file should be written to.
    std::map<std::string, std::string> getInitMessage() const override {
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
    
private:
    // The naming of the URL file is hard-coded.
    // In the future, it might be better to have the communicator itself generate the filename and then send it to the job script.
    std::string getUrlFileName(const std::string& jobID) {
        std::string urlFilename = "url-" + jobID + ".txt";
        urlFilenames.push_back(urlFilename);
        return urlFilename;
    }

    std::filesystem::path fileDir;
    std::filesystem::path filePath;
    
    std::vector<std::string> urlFilenames;

    std::chrono::milliseconds pollingCycle;
};

class FilesystemCommunicatorFactory : public JobCommunicatorFactory {
public:
    FilesystemCommunicatorFactory(std::filesystem::path fileDir, std::chrono::milliseconds pollingCycle)
    : fileDir(fileDir), pollingCycle(pollingCycle) {
        std::filesystem::create_directory(fileDir);
    }

    std::unique_ptr<JobCommunicator> create() const override {
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
        } 
        else if (std::filesystem::exists(scriptDefault)) {
            return scriptDefault;
        } 
        else {
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
    
    virtual void spawnServers() = 0;

    // Grant exclusive ownership of a model (with a given name) to a caller.
    virtual std::shared_ptr<umbridge::Model> requestModelAccess(const std::string& modelName) = 0;
    
    // Enables load balancer to run even when not all allocations are ready + initiate regular
    // healthchecks.
    virtual void startUpRoutine() = 0;

    // To initialize the load balancer we first need a list of model names that are available on a server.
    // Typically, this can be achieved by simply running the model code and requesting the model names from the server.
    // Therefore, the implementation can most likely use the same mechanism that is also used for granting model access.
    virtual std::vector<std::string> getModelName(std::string url) const = 0;
    
    virtual std::set<std::string> getModelNameArray() const = 0;
};


// JobModel represents a single named model within an HPC job allocation.
// Multiple JobModel instances can share the same Job (via shared_ptr) when a single
// allocation hosts several named models. Busyness is tracked at the Job level so that
// the entire allocation is considered busy regardless of which named model is running.
class JobModel : public umbridge::Model {
public:
    JobModel(std::shared_ptr<Job> job, std::unique_ptr<umbridge::Model> model)
    : umbridge::Model(model->GetName()), job(std::move(job)), model(std::move(model)) {}

    std::vector<std::size_t> GetInputSizes(const json &config_json = json::parse("{}")) const override {
        auto inputSizes = model->GetInputSizes(config_json);
        job->setBusyness(false);
        return inputSizes;
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

    // Probes whether this named model is still responding.
    // Has to create a new client to connect to the UM-Bridge server because the same HTTP model pointer
    // would contest with the ongoing request and be blocked until the current request finishes.
    bool checkJobModelLiveness(std::string url, std::string modelName) const {
        httplib::Client client{url.c_str()};
        httplib::Headers headers{httplib::Headers()};
        
        json request_body;
        request_body["name"] = modelName;
        
        if (auto res = client.Post("/InputSizes", headers, request_body.dump(), "application/json")) {
            return true;
        }
        else {
            std::cout << "Named model '" << GetName() << "' in allocation " << job->getJobId() << " is no longer running." << std::endl;
            return false;
        }
    }

    bool checkJobBusyness() const {
        return job->getBusyness();
    }
    
    void setJobBusyness(bool busyness) {
        job->setBusyness(busyness);
    }

private:
    std::shared_ptr<Job> job;
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
        int numServer,
        int idleTimeout) 
        : jobSubmitter(std::move(jobSubmitter)), jobCommFactory(std::move(jobCommFactory)), locator(std::move(locator)), numServer(numServer), idleTimeout(idleTimeout) {}

    std::shared_ptr<umbridge::Model> requestModelAccess(const std::string& modelName) override {
        std::unique_lock lock{serverMutex};

        requestQueue.push(std::this_thread::get_id());

        serverCV.wait(lock, [&] {
            return requestQueue.front() == std::this_thread::get_id() &&
                   hasAvailableServer(modelName);
        });
        requestQueue.pop();

        // Find the first free and alive JobModel for the requested name and mark it busy.
        auto range = serverArray.equal_range(modelName);
        for (auto it = range.first; it != range.second; ++it) {
            auto& [jobModel, url] = it->second;
            if (!jobModel->checkJobBusyness()) {
                jobModel->setJobBusyness(true);
                return jobModel;
            }
        }
        return nullptr; // To remove compiler warning
    }

    void spawnServers() override {
        std::filesystem::path jobScript = locator.getDefaultJobScript();
        std::unique_ptr<JobCommunicator> comm = jobCommFactory->create();
        std::string jobID = jobSubmitter->submit(numServer, jobScript, comm->getInitMessage());
        for (int i = 1; i <= numServer; i++) {
            std::string jobArrayID = jobID + "_" + std::to_string(i);
            std::string url = comm->getModelUrl(jobArrayID);
            auto names = getModelName(url);

            // All named models in the same allocation share one SlurmJob instance.
            auto job = std::make_shared<SlurmJob>(jobArrayID);
            job->setFreeNotifier([this] () { this->serverCV.notify_all(); });

            {
                std::unique_lock lock{serverMutex};
                for (const auto& name : names) {
                    modelNames.insert(name);
                    auto model = std::make_unique<umbridge::HTTPModel>(url, name);
                    auto jobModel = std::make_shared<JobModel>(job, std::move(model));
                    serverArray.emplace(name, std::make_pair(jobModel, url));
                }
            }
            
            serverCV.notify_all();
        }
    }
    
    void startUpRoutine() override {
        waitForFirstServer();
        initiateHealthCheck();
    }
    
    std::vector<std::string> getModelName(std::string url) const override {
        return umbridge::SupportedModels(url);
    }

    std::set<std::string> getModelNameArray() const override {
        return modelNames;
    }
    
    ~CommandJobManager() {
        stopHealthCheck = true;
        healthCheckThread.join();
    }

private:
    // Probes liveness of every JobModel.
    // Dead entries are erased immediately. When the last JobModel sharing a Job is
    // erased, the shared_ptr refcount drops to zero and SlurmJob's destructor fires,
    // cancelling the allocation automatically. Ideally this only happens when time runs out.
    // Unable to handle server crashes yet.
    // Must be called with serverMutex held.
    // The for loop can be rewritten using std::erase_if.
    void checkServerArrayLiveness() {
        std::unique_lock lock{serverMutex};
        for (auto it = serverArray.begin(); it != serverArray.end();) {
            auto& [jobModel, url] = it->second;
            
            bool alive = jobModel->checkJobModelLiveness(url, it->first);
            
            if (alive) {
                ++it;
            }
            else {
                it = serverArray.erase(it);
            }
        }
        serverCV.notify_all();
    }

    // Returns true if at least one JobModel for the requested name is free and alive.
    // Must be called with serverMutex held.
    bool hasAvailableServer(const std::string& modelName) const {
        auto range = serverArray.equal_range(modelName);
        for (auto it = range.first; it != range.second; ++it) {
            const auto& [jobModel, url] = it->second; // Retrieve tuple from std::pair
            if (!jobModel->checkJobBusyness()) return true;
        }
        return false;
    }
    
    void initiateHealthCheck() {
        healthCheckThread = std::thread([this] () {
            while (!stopHealthCheck) {
                std::this_thread::sleep_for(healthCheckInterval);
                
                checkServerArrayLiveness();

                if (serverArray.empty()) {
                    throw std::runtime_error("No alive UM-Bridge servers");
                }
                // TODO: If the queue has been empty for longer than the idle timeout
                // and there are no busy servers. End SLURM allocation and LB
                /*
                if idleTimeout > 0{
                    if (idleConditions) {
                        // handle the idling here
                    }
                }
                */
                serverCV.notify_all();
            }
        });
    }
    
    void waitForFirstServer() {
        std::unique_lock lock{serverMutex};
        serverCV.wait(lock, [this] () { return !serverArray.empty(); });
    }

    std::mutex serverMutex;
    std::condition_variable serverCV;
    std::thread healthCheckThread;
    std::queue<std::thread::id> requestQueue;
    std::unique_ptr<JobSubmitter> jobSubmitter;
    std::unique_ptr<JobCommunicatorFactory> jobCommFactory;
    JobScriptLocator locator;
    int idleTimeout;
    int numServer;
    bool stopHealthCheck = false;
    std::chrono::seconds healthCheckInterval{900}; // Health check every 15 minutes
    // Keyed by model name; multiple entries per name when multiple allocations serve it.
    // Value: (JobModel, url). JobModels sharing the same allocation share a Job via shared_ptr.
    std::multimap<std::string, std::pair<std::shared_ptr<JobModel>, std::string>> serverArray;
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
