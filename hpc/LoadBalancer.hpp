#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <tuple>
#include <memory>
#include <filesystem>
#include "lib/umbridge.h"
#include "HPCIO.hpp"
#include <atomic>

// run and get the result of command
std::string getCommandOutput(const std::string command)
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

std::string readUrl(const std::string &filename)
{
    std::ifstream file(filename);
    std::string url;
    if (file.is_open())
    {
        std::string file_contents((std::istreambuf_iterator<char>(file)),
                                  (std::istreambuf_iterator<char>()));
        url = file_contents;
        file.close();
    }
    else
    {
        std::cerr << "Unable to open file " << filename << " ." << std::endl;
    }

    // delete the line break
    if (!url.empty())
        url.pop_back();

    return url;
}

// state = ["WAITING", "RUNNING", "FINISHED", "CANCELED"]
bool waitForHQJobState(const std::string &job_id, const std::string &state = "COMPLETED")
{
    const std::string command = "hq job info " + job_id + " | grep State | awk '{print $4}'";
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
        // std::cout<<"Job status: "<<job_status<<std::endl;
        sleep(1);
    } while (job_status != state);

    return true;
}

std::string submitHQJob()
{
    std::string hq_command = "hq submit --output-mode=quiet hq_scripts/job.sh";

    std::string job_id = getCommandOutput(hq_command);

    // Delete the line break
    if (!job_id.empty())
        job_id.pop_back();

    std::cout << "Waiting for job " << job_id << " to start." << std::endl;
    
    // Wait for the HQ Job to start
    waitForHQJobState(job_id, "RUNNING"); 

    // Also wait until job is running and url file is written
    waitForFile("./urls/url-" + job_id + ".txt");

    std::cout << "Job " << job_id << " started." << std::endl;

    return job_id;
}

class HyperQueueJob
{
public:
    HyperQueueJob(std::string model_name, bool start_client=true)
    {
        job_id = submitHQJob();

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
        std::system(("hq job cancel " + job_id).c_str());

        // Delete the url text file
        std::system(("rm ./urls/url-" + job_id + ".txt").c_str());
    }

    std::string server_url;
    std::unique_ptr<umbridge::HTTPModel> client_ptr;

private:
    std::string job_id;
};


class LoadBalancer : public umbridge::Model
{

    static std::atomic<int> job_id_counter;

public:
    LoadBalancer(std::string name) : umbridge::Model(name) {}

    std::vector<std::size_t> GetInputSizes(const json &config_json = json::parse("{}")) const override
    {        
        int job_id = job_id_counter++;

        // Write the inputs to a file
        std::string filename = "hpcio-" + std::to_string(job_id) + "-input.txt";
        FileWriter writer(filename);
        writer.writeString("GetInputSizes");
        writer.writeString(name);
        writer.close();


        std::string cmd = "sbatch --export=HPCIO_JOB_ID=" + std::to_string(job_id) + " HPCIO.slurm";
        std::system(cmd.c_str());

        std::string output_filename = "hpcio-" + std::to_string(job_id) + "-output.txt";
        waitForFile(output_filename);

        // Read the output file
        FileReader reader(output_filename);
        std::vector<std::size_t> sizes = reader.readVectorSizeT();
        reader.close();
        //std::filesystem::remove(filename);
        //std::filesystem::remove(output_filename);
        return sizes;
    }

    std::vector<std::size_t> GetOutputSizes(const json &config_json = json::parse("{}")) const override
    {
        int job_id = job_id_counter++;

        // Write the inputs to a file
        std::string filename = "hpcio-" + std::to_string(job_id) + "-input.txt";
        FileWriter writer(filename);
        writer.writeString("GetOutputSizes");
        writer.writeString(name);
        writer.close();

        std::string cmd = "sbatch --export=HPCIO_JOB_ID=" + std::to_string(job_id) + " HPCIO.slurm";
        std::system(cmd.c_str());

        std::string output_filename = "hpcio-" + std::to_string(job_id) + "-output.txt";
        waitForFile(output_filename);

        // Read the output file
        FileReader reader(output_filename);
        std::vector<std::size_t> sizes = reader.readVectorSizeT();
        reader.close();
        //std::filesystem::remove(filename);
        //std::filesystem::remove(output_filename);
        return sizes;
    }

    std::vector<std::vector<double>> Evaluate(const std::vector<std::vector<double>> &inputs, json config_json = json::parse("{}")) override
    {
        int job_id = job_id_counter++;

        // Write the inputs to a file
        std::string filename = "hpcio-" + std::to_string(job_id) + "-input.txt";
        FileWriter writer(filename);
        writer.writeString("Evaluate");
        writer.writeString(name);
        writer.writeVectorVectorDouble(inputs);
        writer.close();

        std::string cmd = "sbatch --export=HPCIO_JOB_ID=" + std::to_string(job_id) + " HPCIO.slurm";
        std::system(cmd.c_str());

        std::string output_filename = "hpcio-" + std::to_string(job_id) + "-output.txt";
        waitForFile(output_filename);

        // Read the output file
        FileReader reader(output_filename);
        std::vector<std::vector<double>> outputs = reader.readVectorVectorDouble();
        reader.close();
        //std::filesystem::remove(filename);
        //std::filesystem::remove(output_filename);
        return outputs;
    }

    std::vector<double> Gradient(unsigned int outWrt,
                                 unsigned int inWrt,
                                 const std::vector<std::vector<double>> &inputs,
                                 const std::vector<double> &sens,
                                 json config_json = json::parse("{}")) override
    {
        std::cerr << "Gradient is not supported by HPCIO" << std::endl;
        return {};
    }

    std::vector<double> ApplyJacobian(unsigned int outWrt,
                                      unsigned int inWrt,
                                      const std::vector<std::vector<double>> &inputs,
                                      const std::vector<double> &vec,
                                      json config_json = json::parse("{}")) override
    {
        std::cerr << "ApplyJacobian is not supported by HPCIO" << std::endl;
        return {};
    }

    std::vector<double> ApplyHessian(unsigned int outWrt,
                                     unsigned int inWrt1,
                                     unsigned int inWrt2,
                                     const std::vector<std::vector<double>> &inputs,
                                     const std::vector<double> &sens,
                                     const std::vector<double> &vec,
                                     json config_json = json::parse("{}"))
    {
        std::cerr << "ApplyHessian is not supported by HPCIO" << std::endl;
        return {};
    }

    bool SupportsEvaluate() override
    {
        return true; // TODO: Query this from model
    }
    bool SupportsGradient() override
    {
        return false; // TODO: Query this from model
    }
    bool SupportsApplyJacobian() override
    {
        return false; // TODO: Query this from model
    }
    bool SupportsApplyHessian() override
    {
        return false; // TODO: Query this from model
    }
};

std::atomic<int> LoadBalancer::job_id_counter{1};