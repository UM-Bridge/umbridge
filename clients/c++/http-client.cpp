// Needed for HTTPS
//#define CPPHTTPLIB_OPENSSL_SUPPORT

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

  // List supported models
  std::vector<std::string> models = umbridge::SupportedModels(host);
  std::cout << "Supported models: " << std::endl;
  for (auto model : models) {
    std::cout << "  " << model << std::endl;
  }

  // Connect to a model
  umbridge::HTTPModel client(host, "forward");

  // Print out input and output sizes
  std::cout << to_string(client.GetInputSizes()) << std::endl;
  std::cout << to_string(client.GetOutputSizes()) << std::endl;

  // Define a single 2D vector as input parameter
  std::vector<std::vector<double>> inputs {{100.0, 18.0}};

  // Evaluate model for input
  std::vector<std::vector<double>> outputs = client.Evaluate(inputs);
  std::cout << "Output: " << to_string(outputs[0]) << std::endl;

  // And evaluate again, this time specifying config parameters
  json config;
  config["level"] = 1;
  config["vtk_output"] = true;
  client.Evaluate(inputs, config);

  // If model supports Jacobian action,
  // apply Jacobian of output zero with respect to input zero to a vector
  if (client.SupportsApplyJacobian()) {
    std::vector<double> jacobian = client.ApplyJacobian(0, 0, inputs, {1.0, 4.0});
    std::cout << "Jacobian action: " << to_string(jacobian) << std::endl;
  }
}
