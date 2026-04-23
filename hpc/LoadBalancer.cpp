#include "LoadBalancer.hpp"
#include "../lib/umbridge.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>


void clearUrl(const std::filesystem::path& directory) {
    if (!std::filesystem::exists(directory)) {
        return;
    }

    for (auto& file : std::filesystem::directory_iterator(directory)) {
        if (std::regex_match(file.path().filename().string(), std::regex("url-\\d+(?:_\\d+)?\\.txt"))) {
            std::filesystem::remove(file);
        }
    }
}

std::string getArg(const std::vector<std::string>& args, const std::string& argName) {
    // Check if a string matches the format --<argName>=...
    const std::string searchString = "--" + argName + "=";
    auto checkFormat = [&searchString](const std::string& s) { 
        return (s.length() > searchString.length()) && (s.rfind(searchString, 0) == 0); 
    };

    // Return value of the argument or empty string if not found
    if (const auto it = std::find_if(args.begin(), args.end(), checkFormat); it != args.end()) {
        return it->substr(searchString.length());
    }

    return "";
}


int main(int argc, char* argv[]) {
    const std::string urlDirectory = "urls";
    clearUrl(urlDirectory);

    // Process command line args
    std::vector<std::string> args(argv + 1, argv + argc);

    // Delay for job submissions in milliseconds
    std::string delayStr = getArg(args, "delay-ms");
    std::chrono::milliseconds delay = std::chrono::milliseconds::zero();
    if (!delayStr.empty()) {
        delay = std::chrono::milliseconds(std::stoi(delayStr));
    }

    // Load balancer port
    std::string portStr = getArg(args, "port");
    int port = 4242;
    if (portStr.empty()) {
        std::cout << "Argument --port not set! Using port 4242 as default." << std::endl;
    } else {
        port = std::stoi(portStr);
    }

    // Number of servers to spawn
    std::string numServerStr = getArg(args, "num-server");
    int numServer = 1;
    if (numServerStr.empty()) {
        std::cout << "Argument --num-server not set! Spawning one model server as default." << std::endl;
    }
    else {
        numServer = std::stoi(numServerStr);
    }
    
    // How long before LB timeout
    std::string idleTimeoutStr = getArg(args, "idle-timeout");
    int idleTimeout = 0;
    if (idleTimeoutStr.empty()) {
        std::cout << "Argument --idle-timeout not set! Using no timeout as default." << std::endl;
    }
    else {
        idleTimeout = std::stoi(idleTimeoutStr);
    }
    
    // Assemble job manager
    std::filesystem::path scriptDir = "slurm_scripts";
    std::unique_ptr<JobSubmitter> jobSubmitter = std::make_unique<SlurmSubmitter>(delay);

    // Only filesystem communication is implemented. May implement network-based communication in the future.
    // Directory which stores URL files and polling cycle currently hard-coded.
    std::unique_ptr<JobCommunicatorFactory> commFactory
        = std::make_unique<FilesystemCommunicatorFactory>(urlDirectory, std::chrono::milliseconds(500));

    // Location of job scripts and naming currently hard-corded.
    JobScriptLocator locator {scriptDir, "job.sh", "job_", ".sh"};

    std::shared_ptr<JobManager> jobManager = std::make_shared<CommandJobManager>(
        std::move(jobSubmitter), std::move(commFactory), locator, numServer, idleTimeout);
        
    // Start SLURM job arrays asynchronously and wait for first server
    std::thread spawnServersThread([&jobManager] () {
        jobManager->spawnServers();
    });
    spawnServersThread.detach();
    jobManager->startUpRoutine();

    // Initialize load balancer for each available model on the model server.
    std::set<std::string> modelNames = jobManager->getModelNameArray();

    // Inform the user about the available models and the job scripts that will be used.
    // locator.printModelJobScripts(modelNames);    

    // Prepare models and serve via network
    std::vector<LoadBalancer> LBVector;
    for (auto modelName : modelNames) {
        LBVector.emplace_back(modelName, jobManager);
    }

    // umbridge::serveModels currently only accepts raw pointers.
    std::vector<umbridge::Model *> LBPtrVector(LBVector.size());
    std::transform(LBVector.begin(), LBVector.end(), LBPtrVector.begin(),
                   [](LoadBalancer& obj) { return &obj; });

    umbridge::serveModels(LBPtrVector, "0.0.0.0", port, true, false);
}

