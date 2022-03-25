#ifndef UMBRIDGE_EXAMPLE_LINEARMODEL_HPP_
#define UMBRIDGE_EXAMPLE_LINEARMODEL_HPP_

#include "umbridge/Model.hpp"

class LinearModel : public umbridge::Model {
public:

  inline LinearModel(std::vector<std::vector<double> > const& A, std::vector<double> const& b) :
  Model(std::vector<std::size_t>(1, A[0].size()), std::vector<std::size_t>(1, b.size())),
  A(A), b(b)
  {}

  virtual ~LinearModel() = default;

  inline virtual void Evaluate(umbridge::Vectors const& inputs, umbridge::Vectors& outputs) override {
    assert(inputs.size()==inputSizes.size());
    for( std::size_t i=0; i<inputs.size(); ++i ) { assert(inputs[i].size()==inputSizes[i]); }

    outputs.resize(1);
    outputs[0] = b;
    for( std::size_t i=0; i<A.size(); ++i ) {
      for( std::size_t j=0; j<A[i].size(); ++j ) {
        outputs[0][i] += A[i][j]*inputs[0][j];
      }
    }
  }

private:

  std::vector<std::vector<double> > const& A;

  std::vector<double> const& b;
};

#endif
