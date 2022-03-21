#include "umbridge/Pybind11Wrappers.hpp"

namespace py = pybind11;

PYBIND11_MODULE(UMBridge, module) {
  umbridge::python::ModelWrapper(module);
}
