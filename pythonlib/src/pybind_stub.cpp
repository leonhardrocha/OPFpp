#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <memory>
#include <vector>
#include <string>
#include "../include/opf/Node.hpp"
#include "../include/opf/Subgraph.hpp"
#include "../include/opf/file.hpp"

namespace py = pybind11;
using opf::Node;
using opf::Subgraph;

PYBIND11_MODULE(opfpy, m) {
    m.doc() = "OPF C++20 Python bindings";
    m.def("hello", []() { return "Hello from OPF C++!"; });

    // Node<float> binding
    py::class_<Node<float>, std::shared_ptr<Node<float>>>(m, "Node")
        .def(py::init<>())
        .def_property("pathval", &Node<float>::getPathval, &Node<float>::setPathval)
        .def_property("dens", &Node<float>::getDens, &Node<float>::setDens)
        .def_property("radius", &Node<float>::getRadius, &Node<float>::setRadius)
        .def_property("label", &Node<float>::getLabel, &Node<float>::setLabel)
        .def_property("root", &Node<float>::getRoot, &Node<float>::setRoot)
        .def_property("pred", &Node<float>::getPred, &Node<float>::setPred)
        .def_property("truelabel", &Node<float>::getTruelabel, &Node<float>::setTruelabel)
        .def_property("position", &Node<float>::getPosition, &Node<float>::setPosition)
        .def_property("status", &Node<float>::getStatus, &Node<float>::setStatus)
        .def_property("relevant", &Node<float>::getRelevant, &Node<float>::setRelevant)
        .def_property("nplatadj", &Node<float>::getNplatadj, &Node<float>::setNplatadj)
        .def_property("feat",
            [](const Node<float>& n) { return *(n.getFeat()); },
            [](Node<float>& n, const std::vector<float>& v) {
                n.setFeat(std::make_shared<std::vector<float>>(v));
            })
        .def_property("adj",
            [](const Node<float>& n) { return n.getAdj(); },
            [](Node<float>& n, const std::vector<int>& v) {
                n.getAdj() = v;
            })
        .def("add_to_adj", &Node<float>::addToAdj)
        .def("clear_adj", &Node<float>::clearAdj);

    // Subgraph<float> binding
    py::class_<Subgraph<float>>(m, "Subgraph")
        .def(py::init<>())
        .def(py::init<int>())
        .def_property("nfeats", &Subgraph<float>::getNumFeats, &Subgraph<float>::setNumFeats)
        .def_property("bestk", &Subgraph<float>::getBestK, &Subgraph<float>::setBestK)
        .def_property("nlabels", &Subgraph<float>::getNumLabels, &Subgraph<float>::setNumLabels)
        .def_property("df", &Subgraph<float>::getDf, &Subgraph<float>::setDf)
        .def_property("mindens", &Subgraph<float>::getMinDens, &Subgraph<float>::setMinDens)
        .def_property("maxdens", &Subgraph<float>::getMaxDens, &Subgraph<float>::setMaxDens)
        .def_property("K", &Subgraph<float>::getK, &Subgraph<float>::setK)
        .def_property_readonly("nnodes", &Subgraph<float>::getNumNodes)
        .def("get_node",
            static_cast<Node<float>& (Subgraph<float>::*)(int)>(&Subgraph<float>::getNode),
            py::return_value_policy::reference_internal)
        .def("add_node", &Subgraph<float>::addNode)
        .def("get_nodes",
            [](Subgraph<float>& sg) -> std::vector<Node<float>>& {
                return const_cast<std::vector<Node<float>>&>(sg.getNodes());
            },
            py::return_value_policy::reference_internal)
        .def("add_ordered_node", &Subgraph<float>::addOrderedNode)
        .def("clear_ordered_list_of_nodes", &Subgraph<float>::clearOrderedListOfNodes)
        .def_property_readonly("ordered_list_of_nodes", &Subgraph<float>::getOrderedListOfNodes)
        .def("write_model", &Subgraph<float>::writeModel)
        .def_static("read_model", &Subgraph<float>::readModel)
        .def_static("from_original_file", [](const std::string& filename) {
            return opf::ReadSubgraph_original<float>(filename);
        }, py::arg("filename"), "Read a Subgraph from the original OPF binary file format.")
        ;

    // Free functions: OPF training-format file I/O (truelabel/position/pathval/features)
    m.def("read_subgraph", [](const std::string& filename) {
        opf::Subgraph<float> sg;
        opf::readSubgraph<float>(filename, sg);
        return sg;
    }, py::arg("filename"),
       "Read a Subgraph from the OPF training binary format (truelabel, position, pathval, features).");

    m.def("write_subgraph", [](const std::string& filename, const opf::Subgraph<float>& sg) {
        opf::writeSubgraph<float>(filename, sg);
    }, py::arg("filename"), py::arg("subgraph"),
       "Write a Subgraph to the OPF training binary format.");
}
