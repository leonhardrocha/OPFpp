#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include "../include/opf/Node.hpp"
#include "../include/opf/Subgraph.hpp"
#include "../include/opf/file.hpp"
#include "../../include_cpp/opf/Distance.hpp"
#include "../../include_cpp/opf/Utils.hpp"
#include "../../include_cpp/opf/OPF.hpp"

namespace py = pybind11;
using opf::Node;
using opf::Subgraph;
using opf::OPF;

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

    // Supervised OPF workflow class
    py::class_<OPF<float>>(m, "OPF")
        .def(py::init<>())
        .def("train", &OPF<float>::training,
            py::arg("train_subgraph"),
            "Train a supervised OPF model in-place on the training subgraph.")
        .def("classify", &OPF<float>::classifying,
            py::arg("train_subgraph"), py::arg("test_subgraph"),
            "Classify test_subgraph nodes in-place using a trained train_subgraph.")
        .def("learn", &OPF<float>::learning,
            py::arg("train_subgraph"), py::arg("eval_subgraph"), py::arg("n_iterations") = 10,
            "Run iterative OPF learning in-place on train_subgraph using eval_subgraph.")
        .def("accuracy", &OPF<float>::accuracy,
            py::arg("subgraph"),
            "Compute OPF accuracy from node labels vs. truelabels.")
        // Phase 4: unsupervised / semi-supervised
        .def("cluster", &OPF<float>::clustering,
            py::arg("subgraph"),
            "Unsupervised OPF clustering in-place. Requires node dens and adj lists populated.")
        .def("knn_classify", &OPF<float>::knnClassifying,
            py::arg("train_subgraph"), py::arg("test_subgraph"),
            "k-NN OPF classification in-place using per-node radius in train_subgraph.")
        .def("semi_supervised", [](OPF<float>& self,
                                   opf::Subgraph<float>& sg_labeled,
                                   opf::Subgraph<float>& sg_unlabeled,
                                   py::object sg_eval_obj) {
                opf::Subgraph<float>* eval_ptr = nullptr;
                std::unique_ptr<opf::Subgraph<float>> eval_owner;
                if (!sg_eval_obj.is_none()) {
                    eval_owner = std::make_unique<opf::Subgraph<float>>(
                        sg_eval_obj.cast<opf::Subgraph<float>>());
                    eval_ptr = eval_owner.get();
                }
                return self.semiSupervisedLearning(sg_labeled, sg_unlabeled, eval_ptr);
            },
            py::arg("labeled_subgraph"), py::arg("unlabeled_subgraph"),
            py::arg("eval_subgraph") = py::none(),
            "Semi-supervised OPF learning. Returns merged trained subgraph.")
        // Phase 5: utilities
        .def("normalize", &OPF<float>::normalize,
            py::arg("subgraph"),
            "Normalize subgraph features in-place using z-score (mean/std-dev per feature).")
        .def("pruning", &OPF<float>::pruning,
            py::arg("train_subgraph"), py::arg("eval_subgraph"), py::arg("desired_accuracy"),
            "Iteratively prune irrelevant training nodes. desired_accuracy is the maximum "
            "allowed per-iteration accuracy drop (tolerance). Stops when the drop exceeds "
            "the tolerance or after 100 iterations. Returns the fraction of nodes removed.");

    // Propagate cluster labels from each node's root to all tree members
    m.def("propagate_cluster_labels", [](opf::Subgraph<float>& sg) {
        for (int i = 0; i < sg.getNumNodes(); ++i) {
            int root = sg.getNode(i).getRoot();
            if (root == i) {
                sg.getNode(i).setLabel(sg.getNode(i).getTruelabel());
            } else {
                sg.getNode(i).setLabel(sg.getNode(root).getLabel());
            }
        }
    }, py::arg("subgraph"),
       "Propagate each cluster root's label to all nodes in its tree.");

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

    m.def("split_subgraph", [](const opf::Subgraph<float>& original, float percentage_first) {
        opf::Subgraph<float> first;
        opf::Subgraph<float> second;
        opf::Subgraph<float> original_copy = original;
        opf::split<float>(original_copy, first, second, percentage_first);
        return py::make_tuple(first, second);
    }, py::arg("original_subgraph"), py::arg("percentage_first"),
       "Split a subgraph into two label-stratified subgraphs.");

    // Distance functions
    m.def("eucl_dist", &opf::distance::euclDist, "Euclidean distance between two float vectors");
    m.def("chi_squared_dist", &opf::distance::chiSquaredDist, "Chi-Squared distance between two float vectors");
    m.def("manhattan_dist", &opf::distance::manhattanDist, "Manhattan distance between two float vectors");
    m.def("canberra_dist", &opf::distance::canberraDist, "Canberra distance between two float vectors");
    m.def("squared_chord_dist", &opf::distance::squaredChordDist, "Squared Chord distance between two float vectors");
    m.def("squared_chi_squared_dist", &opf::distance::squaredChiSquaredDist, "Squared Chi-Squared distance between two float vectors");
    m.def("bray_curtis_dist", &opf::distance::brayCurtisDist, "Bray-Curtis distance between two float vectors");

    // Phase 5: utility free functions

    m.def("subgraph_info", [](const opf::Subgraph<float>& sg) {
        py::dict info;
        info["nnodes"]  = sg.getNumNodes();
        info["nlabels"] = sg.getNumLabels();
        info["nfeats"]  = sg.getNumFeats();
        return info;
    }, py::arg("subgraph"),
       "Return a dict with nnodes, nlabels, and nfeats for the given subgraph.");

    m.def("k_fold", [](opf::Subgraph<float>& sg, int k) {
        return opf::kFold<float>(sg, k);
    }, py::arg("subgraph"), py::arg("k"),
       "Stratified k-fold partition of a subgraph. Returns a list of k Subgraph objects.");

    m.def("merge_subgraphs", [](const opf::Subgraph<float>& sg1, const opf::Subgraph<float>& sg2) {
        return opf::Subgraph<float>::merge(sg1, sg2);
    }, py::arg("subgraph1"), py::arg("subgraph2"),
       "Merge two subgraphs with the same number of features into one.");

    m.def("compute_distance_matrix", [](const opf::Subgraph<float>& sg, int distance_id) {
        int n = sg.getNumNodes();
        std::vector<std::vector<float>> mat(n, std::vector<float>(n, 0.0f));
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                const auto& fi = *sg.getNode(i).getFeat();
                const auto& fj = *sg.getNode(j).getFeat();
                float d = 0.0f;
                switch (distance_id) {
                    case 1: d = opf::distance::euclDist(fi, fj); break;
                    case 2: d = opf::distance::chiSquaredDist(fi, fj); break;
                    case 3: d = opf::distance::manhattanDist(fi, fj); break;
                    case 4: d = opf::distance::canberraDist(fi, fj); break;
                    case 5: d = opf::distance::squaredChordDist(fi, fj); break;
                    case 6: d = opf::distance::squaredChiSquaredDist(fi, fj); break;
                    case 7: d = opf::distance::brayCurtisDist(fi, fj); break;
                    default: throw std::invalid_argument("Invalid distance_id (must be 1-7).");
                }
                mat[i][j] = d;
                mat[j][i] = d;
            }
        }
        return mat;
    }, py::arg("subgraph"), py::arg("distance_id") = 1,
       "Compute NxN pairwise distance matrix. distance_id: 1=Euclidean, 2=Chi-Square, 3=Manhattan, 4=Canberra, 5=SquaredChord, 6=SquaredChiSquared, 7=BrayCurtis.");

    m.def("write_distance_matrix", [](const std::string& filename,
                                      const opf::Subgraph<float>& sg,
                                      int distance_id) {
        int n = sg.getNumNodes();
        std::vector<std::vector<float>> mat(n, std::vector<float>(n, 0.0f));
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                const auto& fi = *sg.getNode(i).getFeat();
                const auto& fj = *sg.getNode(j).getFeat();
                float d = 0.0f;
                switch (distance_id) {
                    case 1: d = opf::distance::euclDist(fi, fj); break;
                    case 2: d = opf::distance::chiSquaredDist(fi, fj); break;
                    case 3: d = opf::distance::manhattanDist(fi, fj); break;
                    case 4: d = opf::distance::canberraDist(fi, fj); break;
                    case 5: d = opf::distance::squaredChordDist(fi, fj); break;
                    case 6: d = opf::distance::squaredChiSquaredDist(fi, fj); break;
                    case 7: d = opf::distance::brayCurtisDist(fi, fj); break;
                    default: throw std::invalid_argument("Invalid distance_id (must be 1-7).");
                }
                mat[i][j] = d;
                mat[j][i] = d;
            }
        }
        std::ofstream out(filename, std::ios::binary);
        if (!out.is_open()) throw std::runtime_error("Cannot open file: " + filename);
        out.write(reinterpret_cast<const char*>(&n), sizeof(int));
        for (int i = 0; i < n; ++i)
            out.write(reinterpret_cast<const char*>(mat[i].data()), n * sizeof(float));
    }, py::arg("filename"), py::arg("subgraph"), py::arg("distance_id") = 1,
       "Compute and write pairwise distance matrix to a binary file (int nnodes + float NxN).");

    m.def("read_distance_matrix", [](const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open()) throw std::runtime_error("Cannot open file: " + filename);
        int n = 0;
        in.read(reinterpret_cast<char*>(&n), sizeof(int));
        std::vector<std::vector<float>> mat(n, std::vector<float>(n));
        for (int i = 0; i < n; ++i)
            in.read(reinterpret_cast<char*>(mat[i].data()), n * sizeof(float));
        return mat;
    }, py::arg("filename"),
       "Read a precomputed distance matrix from a binary file. Returns list of lists of float.");
}
