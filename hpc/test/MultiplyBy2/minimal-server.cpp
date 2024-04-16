#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../../../lib/umbridge.h"


bool isPortInUse(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return false;
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        close(sockfd);
        return true;
    }

    close(sockfd);
    return false;
}

class ExampleModel : public umbridge::Model
{
public:
    ExampleModel(int test_delay, std::string name = "forward")
        : umbridge::Model(name),
          test_delay(test_delay)
    {
    }

    // Define input and output dimensions of model (here we have a single vector of length 1 for input; same for output)
    std::vector<std::size_t> GetInputSizes(const json &config_json) const override
    {
        return {1};
    }

    std::vector<std::size_t> GetOutputSizes(const json &config_json) const override
    {
        return {1};
    }

    std::vector<std::vector<double>> Evaluate(const std::vector<std::vector<double>> &inputs, json config) override
    {
        // Do the actual model evaluation; here we just multiply the first entry of the first input vector by two, and store the result in the output.
        // In addition, we support an artificial delay here, simulating actual work being done.
        std::this_thread::sleep_for(std::chrono::milliseconds(test_delay));

        return {{inputs[0][0] * 2.0}};
    }

    // Specify that our model supports evaluation. Jacobian support etc. may be indicated similarly.
    bool SupportsEvaluate() override
    {
        return true;
    }

private:
    int test_delay;
};

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

int main(int argc, char *argv[])
{

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
        std::cout << "Using port [ " << port_cstr << " ] as specified by environment variable PORT." << std::endl;
        if (isPortInUse(port))
        {
            std::cerr << "Port " << port << " is already in use. Exiting." << std::endl;
            
            exit(-1);
        }

    }

    char const *delay_cstr = std::getenv("TEST_DELAY");
    int test_delay = 0;
    if (delay_cstr != NULL)
    {
        test_delay = atoi(delay_cstr);
    }
    std::cout << "Evaluation delay set to " << test_delay << " ms." << std::endl;

    // Set up and serve model
    ExampleModel model(test_delay);
    ExampleModel model2(15, "backward");
    ExampleModel model3(10, "inward");
    ExampleModel model4(5, "outward");

    std::string hostname = "0.0.0.0";
    /*
    if (argc == 2)
    {
        hostname = argv[1];
    }
    else
    {
        hostname = getCommandOutput("hostname"); // get the hostname of node
        // delete the line break
        if (!hostname.empty())
            hostname.pop_back();
    }
    */
    std::cout << "Hosting server at : "
              << "http://" << hostname << ":" << port << std::endl;
    umbridge::serveModels({&model,&model2,&model3,&model4}, hostname, port); // start server at the hostname

    return 0;
}
