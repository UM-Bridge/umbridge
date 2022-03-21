#ifndef UMBRIDGE_PYBIND11_WRAPPERS_HPP_
#define UMBRIDGE_PYBIND11_WRAPPERS_HPP_

#include <pybind11/pybind11.h>

namespace umbridge {
namespace python {

/// Implement the python interface for the clf::Model class
/**
@param[in] mod The module that holds the python interface
*/
void ModelWrapper(pybind11::module& mod);

} // namespace python
} // namespace umbridge

#endif
