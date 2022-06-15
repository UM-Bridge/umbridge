#include <iostream>

#include <string>

//#include <resolv.h> // Header included in httplib.h, causing potential issues with Eigen!

// Needed for HTTPS
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "umbridge.h"

#include <chrono>
#include <thread>

class ExampleModel : public umbridge::Model {
public:

  ExampleModel(int test_delay)
   : umbridge::Model({1}, {1}), test_delay(test_delay)
  {
    outputs.push_back(std::vector<double>(1));
  }

  void Evaluate(const std::vector<std::vector<double>>& inputs, json config) override {
    std::this_thread::sleep_for(std::chrono::milliseconds(test_delay));
    outputs[0][0] = inputs[0][0] * 2;
  }

  bool SupportsEvaluate() override {
    return true;
  }

private:
  int test_delay;
};

int main(){

  // Read environment variables for configuration
  char const* port_cstr = std::getenv("PORT");
  int port = 0;
  if ( port_cstr == NULL ) {
    std::cout << "Environment variable PORT not set! Using port 4242 as default." << std::endl;
    port = 4242;
  } else {
    port = atoi(port_cstr);
  }

  char const* delay_cstr = std::getenv("TEST_DELAY");
  int test_delay = 0;
  if ( delay_cstr != NULL ) {
    test_delay = atoi(delay_cstr);
  }
  std::cout << "Evaluation delay set to " << test_delay << " ms." << std::endl;


  // Set up and serve model
  ExampleModel model(test_delay);

  umbridge::serveModel(model, "0.0.0.0", port);

  return 0;
}
