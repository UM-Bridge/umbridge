#include "LoadBalancer.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>

#include <unistd.h>
#include <limits.h>

#include "lib/umbridge.h"

void create_directory_if_not_existing(std::string directory) {
    if (!std::filesystem::is_directory(directory) || !std::filesystem::exists(directory)) {
        std::filesystem::create_directory(directory);
    }
}

void clear_url(std::string directory) {
    for (auto& file : std::filesystem::directory_iterator(directory)) {
        if (std::regex_match(file.path().filename().string(), std::regex("url-\\d+\\.txt"))) {
            std::filesystem::remove(file);
        }
    }
}

std::string get_hostname() {
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return std::string(hostname);
}

const std::vector<std::string> get_model_names() {
    // Setup HyperQueue server
    std::system("hq server start &");
    sleep(1); // Workaround: give the HQ server enough time to start.

    // Create allocation queue
    std::system("hq_scripts/allocation_queue.sh");

    HyperQueueJob hq_job("", false); // Don't start a client.

    return umbridge::SupportedModels(hq_job.server_url);
}

int main(int argc, char *argv[])
{
    create_directory_if_not_existing("urls");
    create_directory_if_not_existing("sub-jobs");
    clear_url("urls");
    std::system("hq server stop &> /dev/null");

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

    // Initialize load balancer for each available model on the model server.
    const std::vector<std::string> model_names = get_model_names();

    std::vector<LoadBalancer> LB_vector;
    for (auto model_name : model_names)
    {
        // Set up and serve model
        LB_vector.emplace_back(LoadBalancer{model_name});
    }

    // umbridge::serveModels currently only accepts raw pointers.
    std::vector<umbridge::Model *> LB_ptr_vector(LB_vector.size());
    std::transform(LB_vector.begin(), LB_vector.end(), LB_ptr_vector.begin(),
                   [](LoadBalancer& obj) { return &obj; });

    std::cout << "Load balancer running on host " << get_hostname()
              << " and bound to 0.0.0.0:" << port << std::endl;
    umbridge::serveModels(LB_ptr_vector, "0.0.0.0", port, false);
}
