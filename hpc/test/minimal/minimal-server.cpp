#include <iostream>
#include <string>
#include <chrono>
#include <thread>

// Needed for HTTPS, implies the need for openssl, may be omitted if HTTP suffices
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "../../lib/umbridge.h"

class ExampleModel : public umbridge::Model
{
public:
    ExampleModel(int test_delay)
        : umbridge::Model("forward"),
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

int main()
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
    umbridge::serveModels({&model}, "0.0.0.0", port);

    return 0;
}