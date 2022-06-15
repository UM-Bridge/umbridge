// Needed for HTTPS
#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "umbridge.h"

template<typename T>
std::string to_string(const std::vector<T>& vector) {
  std::string s;
  for (auto entry : vector)
      s += (s.empty() ? "" : ",") + std::to_string(entry);
  return s;
}


int main(int argc, char** argv) {
  if (argc <= 1) {
    std::cout << "Expecting address to connect to as argument, e.g. http://localhost:4242" << std::endl;
    exit(-1);
  }
  std::string host = argv[1];
  std::cout << "Connecting to host " << host << std::endl;

  umbridge::HTTPModel client(host);

  // Print out input and output sizes
  std::cout << to_string(client.inputSizes) << std::endl;
  std::cout << to_string(client.outputSizes) << std::endl;

  // Define a single 2D vector as input parameter
  std::vector<std::vector<double>> inputs {{100.0, 18.0}};

  // Evaluate model for input
  client.Evaluate(inputs);
  std::cout << "Output: " << to_string(client.outputs[0]) << std::endl;

  // And evaluate again, this time specifying config parameters
  json config;
  config["level"] = 0;
  client.Evaluate(inputs, config);
}
