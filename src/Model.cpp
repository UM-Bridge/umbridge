#include "umbridge/Model.hpp"

using namespace umbridge;

Model::Model(std::vector<std::size_t> const& inputSizes, std::vector<std::size_t> const& outputSizes) : inputSizes(inputSizes), outputSizes(outputSizes) {}
