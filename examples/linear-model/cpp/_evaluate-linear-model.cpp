#include <iostream>

#include "umbridge/external/json.hpp"
#include "umbridge/external/httplib.h"

#include "umbridge/ClientModel.hpp"

using namespace nlohmann;
using namespace umbridge;

int main() {
  ClientModel model("http://localhost:4242");

  Vectors inputs(model.inputSizes.size());
  inputs[0].resize(model.inputSizes[0]);
  std::generate(inputs[0].begin(), inputs[0].end(), []() { return (double)rand()/RAND_MAX; });

  Vectors outputs;
  model.Evaluate(inputs, outputs);
  assert(outputs.size()==1);
  assert(outputs[0].size()==model.outputSizes[0]);

  std::cout << "output: ";
  for( std::size_t i=0; i<outputs[0].size(); ++i ) { std::cout << outputs[0][i] << " "; }
  std::cout << std::endl;
}
