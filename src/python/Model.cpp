#include <pybind11/pybind11.h>

#include "umbridge/Pybind11Wrappers.hpp"

#include "umbridge/Model.hpp"

namespace py = pybind11;
using namespace umbridge;

void umbridge::python::ModelWrapper(pybind11::module& mod) {
  py::class_<Model, std::shared_ptr<Model> > model(mod, "Model");
  model.def(py::init<>());
  //model.def(py::init( [] () { return new Model(); }));
}
