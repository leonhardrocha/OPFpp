#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include "opf/OPFpp.hpp"

namespace py = pybind11;

PYBIND11_MODULE(pyopf, m) {
    m.doc() = "Python bindings for the modern C++ LibOPF library";

    //=========================================================================
    // SNode Binding
    //=========================================================================
    py::class_<opf::SNode>(m, "SNode", "Represents a single node in the graph.")
        .def(py::init<size_t>(), py::arg("n_feats") = 0)
        .def_readwrite("feat", &opf::SNode::feat, "Feature vector of the node.")
        .def_readwrite("truelabel", &opf::SNode::truelabel, "The ground-truth label of the node.")
        .def_readwrite("label", &opf::SNode::label, "The assigned label after training or classification.")
        .def_readwrite("pathval", &opf::SNode::pathval, "The path value (cost) from its prototype.")
        .def_readwrite("dens", &opf::SNode::dens, "The density of the node.")
        .def_readwrite("pred", &opf::SNode::pred, "The predecessor node in the optimum-path tree.")
        .def_readwrite("position", &opf::SNode::position, "Original position or ID of the node.")
        .def("__repr__", [](const opf::SNode &n) {
            return "<pyopf.SNode with label " + std::to_string(n.label) + ">";
        });

    //=========================================================================
    // SgCTNode Binding
    //=========================================================================
    py::class_<opf::SgCTNode>(m, "SgCTNode", "Represents a node in the component tree.")
        .def(py::init<>())
        .def_readonly("level", &opf::SgCTNode::level, "Gray level of the component.")
        .def_readonly("comp", &opf::SgCTNode::comp, "Representative pixel of this component.")
        .def_readonly("dad", &opf::SgCTNode::dad, "Parent node in the component tree.")
        .def_readonly("son", &opf::SgCTNode::son, "List of child nodes.")
        .def_readonly("size", &opf::SgCTNode::size, "Number of pixels in the component.")
        .def("__repr__", [](const opf::SgCTNode &n) {
            return "<pyopf.SgCTNode at level " + std::to_string(n.level) +
                   " with size " + std::to_string(n.size) + ">";
        });

    //=========================================================================
    // Subgraph Binding
    //=========================================================================
    py::class_<opf::Subgraph>(m, "Subgraph", "Represents a graph of nodes for OPF.")
        .def(py::init<size_t, size_t, size_t>(),
             py::arg("n_nodes") = 0, py::arg("n_feats") = 0, py::arg("n_labels") = 0,
             "Subgraph constructor.")

        // --- Public members ---
        .def_readwrite("nodes", &opf::Subgraph::nodes, "A list of all SNode objects in the graph.")
        .def_readonly("nlabels", &opf::Subgraph::nlabels, "Number of labels in the dataset.")
        .def_readonly("nfeats", &opf::Subgraph::nfeats, "Number of features per node.")

        // --- Static I/O methods ---
        .def_static("read", &opf::Subgraph::Read, py::arg("filename"),
                    "Reads a subgraph from a binary .dat file.")
        .def_static("read_from_text", &opf::Subgraph::ReadFromText, py::arg("filename"),
                    "Reads a subgraph from a human-readable text file.")

        // --- Member I/O methods ---
        .def("write", &opf::Subgraph::Write, py::arg("filename"),
             "Writes the subgraph to a binary .dat file.")
        .def("write_as_text", &opf::Subgraph::WriteAsText, py::arg("filename"),
             "Writes the subgraph to a human-readable text file.")

        // --- Algorithms ---
        .def("train", &opf::Subgraph::Train, "Trains the OPF classifier on this subgraph.")
        .def("classify", &opf::Subgraph::Classify, py::arg("test_subgraph"),
             "Classifies a test subgraph using this trained model. The test subgraph is modified in-place.")
        .def("normalize_features", &opf::Subgraph::NormalizeFeatures,
             "Normalizes features to have zero mean and unit variance.")
        .def("cluster", &opf::Subgraph::Cluster,
             py::arg("k_max"),
             py::arg("filter_type") = opf::Subgraph::UnsupervisedFilterType::None,
             py::arg("filter_param") = 0.0f,
             "Performs unsupervised clustering.\n\n"
             "This method finds the best k (up to k_max), optionally filters weak clusters,\n"
             "and assigns a cluster label to each node. The subgraph is modified in-place.")

        // --- Data Manipulation ---
        // The C++ Split method is destructive. We wrap it to provide a non-destructive,
        // Pythonic interface that returns two new subgraphs.
        .def("split", [](const opf::Subgraph &self, float training_percentage) {
            auto self_copy = std::make_unique<opf::Subgraph>(self);
            auto testing_part = self_copy->Split(training_percentage);
            // After the split, self_copy contains the training part.
            return std::make_tuple(std::move(self_copy), std::move(testing_part));
        }, py::arg("training_percentage"),
           "Splits the subgraph into two parts. Returns a tuple of (training_subgraph, testing_subgraph).")

        .def("__len__", [](const opf::Subgraph &g) { return g.nodes.size(); })
        .def("__repr__", [](const opf::Subgraph &g) {
            return "<pyopf.Subgraph with " + std::to_string(g.nodes.size()) + " nodes>";
        });

    py::enum_<opf::Subgraph::UnsupervisedFilterType>(m, "UnsupervisedFilterType")
        .value("NONE", opf::Subgraph::UnsupervisedFilterType::None)
        .value("HEIGHT", opf::Subgraph::UnsupervisedFilterType::Height)
        .value("AREA", opf::Subgraph::UnsupervisedFilterType::Area)
        .value("VOLUME", opf::Subgraph::UnsupervisedFilterType::Volume)
        .export_values();

    //=========================================================================
    // SgCTree Binding
    //=========================================================================
    py::class_<opf::SgCTree>(m, "SgCTree", "Component Tree for advanced filtering.")
        .def(py::init<const opf::Subgraph&>(), py::arg("subgraph"),
             "Builds a component tree from a given subgraph.")

        .def_readonly("nodes", &opf::SgCTree::nodes, "List of SgCTNode objects in the tree.")
        .def_readonly("cmap", &opf::SgCTree::cmap, "Component map from original graph nodes to tree nodes.")
        .def_readonly("root", &opf::SgCTree::root, "Index of the root node of the tree.")

        .def_static("area_open", &opf::SgCTree::AreaOpen, py::arg("subgraph"), py::arg("threshold"),
                    "Performs an area-opening filter on the subgraph's density.")

        .def_static("volume_open", &opf::SgCTree::VolumeOpen, py::arg("subgraph"), py::arg("threshold"),
                    "Performs a volume-opening filter on the subgraph's density.")

        .def("__repr__", [](const opf::SgCTree &t) {
            return "<pyopf.SgCTree with " + std::to_string(t.nodes.size()) + " nodes>";
        });
}