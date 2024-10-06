#include "LoadBalancer.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include <algorithm>

#include <unistd.h>
#include <limits.h>

#include "../lib/umbridge.h"

void clear_url(std::string directory) {
    for (auto& file : std::filesystem::directory_iterator(directory)) {
        if (std::regex_match(file.path().filename().string(), std::regex("url-\\d+\\.txt"))) {
            std::filesystem::remove(file);
        }
    }
}

void launch_hq_with_alloc_queue() {
    std::system("./hq server stop &> /dev/null");

    std::system("until ./hq server info &> /dev/null; do sleep 1; done");

    // Create HQ allocation queue
    std::system("hq_scripts/allocation_queue.sh");
}


int main(int argc, char *argv[])
{
    clear_url("urls");

    launch_hq_with_alloc_queue();

    // Read environment variables for configuration
    char const *port_cstr = std::getenv("PORT");
    int port = 0;
    if (port_cstr == NULL)
    {
        std::cout << "Environment variable PORT not set! Using port 4242 as default." << std::endl;
        port = 4242;
    }
    else
    {
        port = atoi(port_cstr);
    }

    JobScriptLocator locator {"hq_scripts", "job.sh", "job_", ".sh"};
    std::shared_ptr<JobManager> job_manager = std::make_shared<CommandJobManager>(
        std::make_unique<HyperQueueSubmitter>(std::chrono::milliseconds(100)),
        std::make_unique<FilesystemCommunicatorFactory>("urls", std::chrono::milliseconds(100)),
        locator
    );

    // Initialize load balancer for each available model on the model server.
    std::vector<std::string> model_names = job_manager->getModelNames();

    // Inform the user about the available models and the job scripts that will be used.
    locator.printModelJobScripts(model_names);    

    std::vector<LoadBalancer> LB_vector;
    for (auto model_name : model_names)
    {
        // Set up and serve model
        LB_vector.emplace_back(model_name, job_manager);
    }

    // umbridge::serveModels currently only accepts raw pointers.
    std::vector<umbridge::Model *> LB_ptr_vector(LB_vector.size());
    std::transform(LB_vector.begin(), LB_vector.end(), LB_ptr_vector.begin(),
                   [](LoadBalancer& obj) { return &obj; });

    std::cout << "Load balancer running port" << port << std::endl;
    umbridge::serveModels(LB_ptr_vector, "0.0.0.0", port, true, false);
}
