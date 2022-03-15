#include <iostream>

#include <string>

//#include <resolv.h> // Header included in httplib.h, causing potential issues with Eigen!

// Needed for HTTPS
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "umbridge.h"

#include <chrono>
#include <thread>

int test_delay = 0;

class ExampleModel : public umbridge::Model {
public:

  ExampleModPiece()
   : umbridge::Model(Eigen::VectorXi::Ones(1)*1, Eigen::VectorXi::Ones(1))
  {
    outputs.push_back(Eigen::VectorXd::Ones(1));
  }

  void Evaluate(std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs, json config) override {
    std::this_thread::sleep_for(std::chrono::seconds(test_delay));
    outputs[0][0] = (inputs[0].get())[0] * 2;
  }

  bool SupportsEvaluate() override {
    return true;
  }
};

int main(){

  char const* port_cstr = std::getenv("PORT");
  int port = 0;
  if ( port_cstr == NULL ) {
    std::cout << "Environment variable PORT not set! Using port 4242 as default." << std::endl;
    port = 4242;
  } else {
    port = atoi(port_cstr);
  }

  char const* delay_cstr = std::getenv("TEST_DELAY");
  if ( delay_cstr != NULL ) {
    test_delay = atoi(delay_cstr);
  }



  ExampleModel modPiece;

  umbridge::serveModel(modPiece, "0.0.0.0", port);

  return 0;
}
