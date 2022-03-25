#ifndef UMBRIDGEMODEL_HPP_
#define UMBRIDGEMODEL_HPP_

#include <vector>

namespace umbridge {

typedef std::vector<std::vector<double> > Vectors;

/// A model
class Model {
public:

  Model(std::vector<std::size_t> const& inputSizes, std::vector<std::size_t> const& outputSizes);

  virtual ~Model() = default;

  virtual void Evaluate(Vectors const& inputs, Vectors& outputs) = 0;

  const std::vector<std::size_t> inputSizes;

  const std::vector<std::size_t> outputSizes;

private:
};

} // namespace umbridge

#endif
