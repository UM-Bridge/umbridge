#include "LoadBalancer.hpp"
#include "../lib/umbridge.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>


void clear_url(const std::filesystem::path& directory) {
    if (!std::filesystem::exists(directory)) {
        return;
    }

    for (auto& file : std::filesystem::directory_iterator(directory)) {
        if (std::regex_match(file.path().filename().string(), std::regex("url-\\d+\\.txt"))) {
            std::filesystem::remove(file);
        }
    }
}

std::string get_arg(const std::vector<std::string>& args, const std::string& arg_name) {
    // Check if a string matches the format --<arg_name>=...
    const std::string search_string = "--" + arg_name + "=";
    auto check_format = [&search_string](const std::string& s) { 
        return (s.length() > search_string.length()) && (s.rfind(search_string, 0) == 0); 
    };

    // Return value of the argument or empty string if not found
    if (const auto it = std::find_if(args.begin(), args.end(), check_format); it != args.end()) {
        return it->substr(search_string.length());
    }

    return "";
}


int main(int argc, char* argv[]) {
    const std::string url_directory = "urls";
    clear_url(url_directory);

    // Process command line args
    std::vector<std::string> args(argv + 1, argv + argc);

    // Delay for job submissions in milliseconds
    std::string delay_str = get_arg(args, "delay-ms");
    std::chrono::milliseconds delay = std::chrono::milliseconds::zero();
    if (!delay_str.empty()) {
        delay = std::chrono::milliseconds(std::stoi(delay_str));
    }

    // Load balancer port
    std::string port_str = get_arg(args, "port");
    int port = 4242;
    if (port_str.empty()) {
        std::cout << "Argument --port not set! Using port 4242 as default." << std::endl;
    } else {
        port = std::stoi(port_str);
    }

    // Number of servers to spawn
    std::string server_str = get_arg(args, "num-server");
    int num_server = 1;
    if (server_str.empty()) {
        std::cout << "Argument --num-server not set! Spawning one model server as default." << std::endl;
    }
    else {
        num_server = std::stoi(server_str);
    }
    
    // Assemble job manager
    std::unique_ptr<JobSubmitter> job_submitter;
    std::filesystem::path script_dir;
    job_submitter = std::make_unique<SlurmSubmitter>(delay);
    script_dir = "slurm_scripts";

    // Only filesystem communication is implemented. May implement network-based communication in the future.
    // Directory which stores URL files and polling cycle currently hard-coded.
    std::unique_ptr<JobCommunicatorFactory> comm_factory 
        = std::make_unique<FilesystemCommunicatorFactory>(url_directory, std::chrono::milliseconds(500));

    // Location of job scripts and naming currently hard-corded.
    JobScriptLocator locator {script_dir, "job.sh", "job_", ".sh"};

    std::shared_ptr<JobManager> job_manager = std::make_shared<CommandJobManager>(
        std::move(job_submitter), std::move(comm_factory), locator, num_server);


    // Initialize load balancer for each available model on the model server.
    std::set<std::string> model_names = job_manager->getModelNameArray();

    // Inform the user about the available models and the job scripts that will be used.
    // locator.printModelJobScripts(model_names);    

    // Prepare models and serve via network
    std::vector<LoadBalancer> LB_vector;
    for (auto model_name : model_names) {
        LB_vector.emplace_back(model_name, job_manager);
    }

    // umbridge::serveModels currently only accepts raw pointers.
    std::vector<umbridge::Model *> LB_ptr_vector(LB_vector.size());
    std::transform(LB_vector.begin(), LB_vector.end(), LB_ptr_vector.begin(),
                   [](LoadBalancer& obj) { return &obj; });

    umbridge::serveModels(LB_ptr_vector, "0.0.0.0", port, true, false);
}
