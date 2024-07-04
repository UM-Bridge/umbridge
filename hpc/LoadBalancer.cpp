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

void launch_hq_with_alloc_queue() {
    std::system("./hq server stop &> /dev/null");

    std::system("./hq server start &");
    sleep(1); // Workaround: give the HQ server enough time to start.

    // Create HQ allocation queue
    std::system("hq_scripts/allocation_queue.sh");
}

const std::vector<std::string> get_model_names() {
    // Don't start a client, always use the default job submission script.
    HyperQueueJob hq_job("", false, true);

    return umbridge::SupportedModels(hq_job.server_url);
}

void print_model_and_job_script_information(const std::vector<std::string>& model_names) {
    // Constants
    const std::filesystem::path SUBMISSION_SCRIPT_DIR("./hq_scripts");
    const std::filesystem::path SUBMISSION_SCRIPT_GENERIC("job.sh");

    const std::string SECTION_START_DELIMITER = "==============================MODEL INFO==============================";
    const std::string SECTION_END_DELIMITER   = "======================================================================";

    // Sort the model names in alphabetical order for cleaner output.
    std::vector<std::string> model_names_sorted = model_names;
    std::sort(model_names_sorted.begin(), model_names_sorted.end());

    std::cout << SECTION_START_DELIMITER << "\n";
    // Print list of available models and corresponding job-scripts.
    std::cout << "Available models and corresponding job-scripts:\n";
    for (const std::string& model_name : model_names_sorted) {
        // Determine which job script will be used by checking if a model specific job script exists.
        std::string used_job_script;
        const std::filesystem::path submission_script_model_specific("job_" + model_name + ".sh");
        if (std::filesystem::exists(SUBMISSION_SCRIPT_DIR / submission_script_model_specific)) {
            used_job_script = submission_script_model_specific.string();
        } else {
            used_job_script = SUBMISSION_SCRIPT_GENERIC.string();
        }
        std::cout << "* Model '" << model_name << "' --> '" << used_job_script << "'\n";
    }
    std::cout << std::endl;


    // Check if there are job scripts that are unused and print a warning.
    std::vector<std::string> unused_job_scripts;

    // Build a regex to parse job-script filenames and extract the model name.
    // Format should be: job_<model_name>.sh
    const std::string format_prefix = "^job_"; // Ensures that filename starts with 'job_'.
    const std::string format_suffix = "\\.sh$"; // Ensures that filename ends with '.sh'.
    const std::string format_model_name = "(.*)"; // Arbitrary sequence of characters as a marked subexpression.
    const std::regex format_regex(format_prefix + format_model_name + format_suffix);

    for (auto& file : std::filesystem::directory_iterator(SUBMISSION_SCRIPT_DIR)) {
        const std::string filename = file.path().filename().string();
        // Check if filename matches format of a model specific job script, i.e. 'job_<model_name>.sh'.
        std::smatch match_result;
        if (std::regex_search(filename, match_result, format_regex)) {
            // Extract first matched subexpression, i.e. the model name.
            const std::string model_name = match_result[1].str();
            // Check if a corresponding model exists. If not, mark job script as unused.
            if (!std::binary_search(model_names_sorted.begin(), model_names_sorted.end(), model_name)) {
                unused_job_scripts.push_back(filename);
            }
        }
    }

    // Print the warning message.
    if(!unused_job_scripts.empty()) {
        // Sort unused job scripts alphabetically for cleaner output.
        std::sort(unused_job_scripts.begin(), unused_job_scripts.end());

        std::cout << "WARNING: The following model-specific job-scripts are not used by any of the available models:\n";
        for (const std::string& job_script : unused_job_scripts) {
            std::cout << "* '" << job_script << "'\n";
        }
        std::cout << std::endl;

        std::cout << "If this behavior is unintentional, then please verify that:\n"
                  << "1. The filename of your model-specific job-script follows the format: 'job_<your_model_name>.sh' (e.g. 'job_mymodel.sh')\n"
                  << "2. The spelling of your model name matches in the model definition and in the filename of your model-specific job-script.\n";
    }

    std::cout << SECTION_END_DELIMITER << std::endl;
}

std::atomic<int32_t> HyperQueueJob::job_count = 0;

int main(int argc, char *argv[])
{
    create_directory_if_not_existing("urls");
    create_directory_if_not_existing("sub-jobs");
    clear_url("urls");

    // SLURM version
    //launch_hq_with_alloc_queue();

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

    char const *delay_cstr = std::getenv("HQ_SUBMIT_DELAY_MS");
    if (delay_cstr != NULL)
    {
        hq_submit_delay_ms = atoi(delay_cstr);
    }
    std::cout << "HQ_SUBMIT_DELAY_MS set to " << hq_submit_delay_ms << std::endl;

    // Initialize load balancer for each available model on the model server.
    const std::vector<std::string> model_names = get_model_names();

    // Inform the user about the available models and the job scripts that will be used.
    // Output a warning for unused model-specific job-scripts to prevent typos.
    print_model_and_job_script_information(model_names);

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

    std::cout << "Load balancer running port" << port << std::endl;
    umbridge::serveModels(LB_ptr_vector, "0.0.0.0", port, true, false);
}
