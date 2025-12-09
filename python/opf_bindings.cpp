#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(pyopf, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring
}
