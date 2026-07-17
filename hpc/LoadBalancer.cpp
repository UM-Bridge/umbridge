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

void launch_hq_with_alloc_queue() {
    std::system("./hq server stop &> /dev/null");

    std::system("./hq server start &");
    // Wait for the HQ server to start
    std::system("until ./hq server info &> /dev/null; do sleep 1; done");

    // Create HQ allocation queue
    std::system("hq_scripts/allocation_queue.sh");
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

    // Scheduler used by the load balancer (currently either SLURM or HyperQueue)
    std::string scheduler = get_arg(args, "scheduler");
    // Specifying a scheduler is mandatory since this should be a conscious choice by the user
    if (scheduler.empty()) {
        std::cerr << "Missing required argument: --scheduler=[hyperqueue | slurm]" << std::endl;
        std::exit(-1);
    }

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

    
    // Assemble job manager
    std::unique_ptr<JobSubmitter> job_submitter;
    std::filesystem::path script_dir;
    if (scheduler == "hyperqueue") {
        launch_hq_with_alloc_queue();
        job_submitter = std::make_unique<HyperQueueSubmitter>(delay);
        script_dir = "hq_scripts";
    } else if (scheduler == "slurm") {
        job_submitter = std::make_unique<SlurmSubmitter>(delay);
        script_dir = "slurm_scripts";
    } else {
        std::cerr << "Unrecognized value for argument --scheduler: "
                  << "Expected hyperqueue or slurm but got " << scheduler << " instead." << std::endl;
        std::exit(-1);
    }

    // Only filesystem communication is implemented. May implement network-based communication in the future.
    // Directory which stores URL files and polling cycle currently hard-coded.
    std::unique_ptr<JobCommunicatorFactory> comm_factory 
        = std::make_unique<FilesystemCommunicatorFactory>(url_directory, std::chrono::milliseconds(500));

    // Location of job scripts and naming currently hard-corded.
    JobScriptLocator locator {script_dir, "job.sh", "job_", ".sh"};

    std::shared_ptr<JobManager> job_manager = std::make_shared<CommandJobManager>(
        std::move(job_submitter), std::move(comm_factory), locator);


    // Initialize load balancer for each available model on the model server.
    std::vector<std::string> model_names = job_manager->getModelNames();

    // Inform the user about the available models and the job scripts that will be used.
    locator.printModelJobScripts(model_names);    

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
