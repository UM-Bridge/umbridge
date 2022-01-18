// Needed for HTTPS
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "HTTPComm.h"

int main(int argc, char** argv) {
  if (argc <= 1) {
    std::cout << "Expecting address to connect to as argument, e.g. http://localhost:4242" << std::endl;
    exit(-1);
  }
  std::string host = argv[1];
  std::cout << "Connecting to host " << host << std::endl;

  httplib::Headers headers;
  ShallowModPieceClient client(host, headers);

  // Print out input and output sizes
  std::cout << client.inputSizes << std::endl;
  std::cout << client.outputSizes << std::endl;

  // Define a single 2D vector as input parameter
  const Eigen::VectorXd zero = Eigen::VectorXd::Ones(2);
  std::vector input = {std::reference_wrapper(zero)};

  // Evaluate model for input
  client.Evaluate(input);

  // And evaluate again, this time specifying config parameters
  json config;
  config["level"] = 0;
  client.Evaluate(input, config);
}
