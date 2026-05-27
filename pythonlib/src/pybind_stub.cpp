#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include "../include/opf/Node.hpp"
#include "../include/opf/Subgraph.hpp"
#include <opf/KernelSubGraph.hpp>
#include <opf/KernelJointProbability.hpp>
#include "../include/opf/file.hpp"
#include "../include/opf/Distance.hpp"
#include "../include/opf/Utils.hpp"
#include "../include/opf/OPF.hpp"

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
        .def("clear_adj", &Node<float>::clearAdj)
        .def("dtype", [](const Node<float>&) { return "float"; }, "Return the data type of features (\"float\")");

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
            [](Subgraph<float>& sg, int i) -> Node<float>& {
                return sg.getNode(i);
            },
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
        .def("dtype", [](const Subgraph<float>&) { return "float"; }, "Return the data type of features (\"float\")")
        ;

    // Node<double> binding
    py::class_<Node<double>, std::shared_ptr<Node<double>>>(m, "NodeDouble")
        .def(py::init<>())
        .def_property("pathval", &Node<double>::getPathval, &Node<double>::setPathval)
        .def_property("dens", &Node<double>::getDens, &Node<double>::setDens)
        .def_property("radius", &Node<double>::getRadius, &Node<double>::setRadius)
        .def_property("label", &Node<double>::getLabel, &Node<double>::setLabel)
        .def_property("root", &Node<double>::getRoot, &Node<double>::setRoot)
        .def_property("pred", &Node<double>::getPred, &Node<double>::setPred)
        .def_property("truelabel", &Node<double>::getTruelabel, &Node<double>::setTruelabel)
        .def_property("position", &Node<double>::getPosition, &Node<double>::setPosition)
        .def_property("status", &Node<double>::getStatus, &Node<double>::setStatus)
        .def_property("relevant", &Node<double>::getRelevant, &Node<double>::setRelevant)
        .def_property("nplatadj", &Node<double>::getNplatadj, &Node<double>::setNplatadj)
        .def_property("feat",
            [](const Node<double>& n) { return *(n.getFeat()); },
            [](Node<double>& n, const std::vector<double>& v) {
                n.setFeat(std::make_shared<std::vector<double>>(v));
            })
        .def_property("adj",
            [](const Node<double>& n) { return n.getAdj(); },
            [](Node<double>& n, const std::vector<int>& v) {
                n.getAdj() = v;
            })
        .def("add_to_adj", &Node<double>::addToAdj)
        .def("clear_adj", &Node<double>::clearAdj)
        .def("dtype", [](const Node<double>&) { return "double"; }, "Return the data type of features (\"double\")");

    // Subgraph<double> binding
    py::class_<Subgraph<double>>(m, "SubgraphDouble")
        .def(py::init<>())
        .def(py::init<int>())
        .def_property("nfeats", &Subgraph<double>::getNumFeats, &Subgraph<double>::setNumFeats)
        .def_property("bestk", &Subgraph<double>::getBestK, &Subgraph<double>::setBestK)
        .def_property("nlabels", &Subgraph<double>::getNumLabels, &Subgraph<double>::setNumLabels)
        .def_property("df", &Subgraph<double>::getDf, &Subgraph<double>::setDf)
        .def_property("mindens", &Subgraph<double>::getMinDens, &Subgraph<double>::setMinDens)
        .def_property("maxdens", &Subgraph<double>::getMaxDens, &Subgraph<double>::setMaxDens)
        .def_property("K", &Subgraph<double>::getK, &Subgraph<double>::setK)
        .def_property_readonly("nnodes", &Subgraph<double>::getNumNodes)
        .def("get_node",
            [](Subgraph<double>& sg, int i) -> Node<double>& {
                return sg.getNode(i);
            },
            py::return_value_policy::reference_internal)
        .def("add_node", &Subgraph<double>::addNode)
        .def("get_nodes",
            [](Subgraph<double>& sg) -> std::vector<Node<double>>& {
                return const_cast<std::vector<Node<double>>&>(sg.getNodes());
            },
            py::return_value_policy::reference_internal)
        .def("add_ordered_node", &Subgraph<double>::addOrderedNode)
        .def("clear_ordered_list_of_nodes", &Subgraph<double>::clearOrderedListOfNodes)
        .def_property_readonly("ordered_list_of_nodes", &Subgraph<double>::getOrderedListOfNodes)
        .def("write_model", &Subgraph<double>::writeModel)
        .def_static("read_model", &Subgraph<double>::readModel)
        .def_static("from_original_file", [](const std::string& filename) {
            return opf::ReadSubgraph_original<double>(filename);
        }, py::arg("filename"), "Read a Subgraph from the original OPF binary file format.")
        .def("dtype", [](const Subgraph<double>&) { return "double"; }, "Return the data type of features (\"double\")")
        ;

    // Supervised OPF workflow class
    py::enum_<OPF<float>::AdjacencyMode>(m, "AdjacencyMode")
        .value("LEGACY_KNN", OPF<float>::AdjacencyMode::LegacyKnn)
        .value("PRESET", OPF<float>::AdjacencyMode::Preset)
        .export_values();

    py::enum_<OPF<float>::DensityEstimationMode>(m, "DensityEstimationMode")
        .value("GAUSSIAN", OPF<float>::DensityEstimationMode::Gaussian)
        .value("INVERSE_DISTANCE", OPF<float>::DensityEstimationMode::InverseDistance)
        .export_values();

    py::class_<OPF<float>::KernelBestKResult>(m, "KernelBestKResult")
        .def(py::init<>())
        .def_readwrite("kernel_id", &OPF<float>::KernelBestKResult::kernel_id)
        .def_readwrite("bestk", &OPF<float>::KernelBestKResult::bestk)
        .def_readwrite("mincut", &OPF<float>::KernelBestKResult::mincut)
        .def_readwrite("df_at_bestk", &OPF<float>::KernelBestKResult::df_at_bestk);

        py::class_<opf::KernelJointProbabilityAccumulator>(m, "KernelJointProbabilityAccumulator")
               .def(py::init<int, const std::vector<float>&>(),
                   py::arg("nnodes") = 0,
                   py::arg("kernel_weights") = std::vector<float>{})
           .def("get_num_nodes", &opf::KernelJointProbabilityAccumulator::getNumNodes,
               "Get accumulator node dimension.")
           .def("has_kernel", &opf::KernelJointProbabilityAccumulator::hasKernel,
               py::arg("kernel_id"), "Check whether a kernel contribution exists.")
           .def("clear", &opf::KernelJointProbabilityAccumulator::clear,
               "Remove all kernel contributions and zero central sum.")
           .def("reset", &opf::KernelJointProbabilityAccumulator::reset,
               py::arg("nnodes"), "Reset accumulator with a new node dimension.")
               .def("set_kernel_weights", &opf::KernelJointProbabilityAccumulator::setKernelWeights,
                   py::arg("kernel_weights"),
                   "Set all kernel weights (missing kernel IDs default to 1.0).")
               .def("get_kernel_weights", &opf::KernelJointProbabilityAccumulator::getKernelWeights,
                   py::return_value_policy::copy,
                   "Get the current kernel weights vector.")
               .def("set_kernel_weight", &opf::KernelJointProbabilityAccumulator::setKernelWeight,
                   py::arg("kernel_id"), py::arg("weight"),
                   "Set one kernel weight; vector grows with default 1.0 if needed.")
               .def("get_kernel_weight", &opf::KernelJointProbabilityAccumulator::getKernelWeight,
                   py::arg("kernel_id"),
                   "Get one kernel weight (defaults to 1.0 for out-of-range IDs).")
           .def("update_kernel_probabilities", &opf::KernelJointProbabilityAccumulator::updateKernelProbabilities,
               py::arg("kernel_id"), py::arg("new_probs"),
               "Replace-update kernel probabilities: subtract old, add new.")
           .def("remove_kernel_probabilities", &opf::KernelJointProbabilityAccumulator::removeKernelProbabilities,
               py::arg("kernel_id"), "Remove one kernel contribution from central sum.")
           .def("get_central_joint_probabilities", &opf::KernelJointProbabilityAccumulator::getCentralJointProbabilities,
               py::return_value_policy::copy,
               "Get central per-node summed probabilities.")
           .def("get_central_joint_sum", &opf::KernelJointProbabilityAccumulator::getCentralJointSum,
               "Get scalar sum over central per-node probabilities.");

    py::class_<OPF<float>>(m, "OPF")
        .def(py::init<>())
        .def("set_adjacency_mode", &OPF<float>::setAdjacencyMode,
            py::arg("mode"),
            "Set adjacency handling mode for k-NN graph construction.")
        .def("get_adjacency_mode", &OPF<float>::getAdjacencyMode,
            "Get current adjacency handling mode.")
        .def("set_density_estimation_mode", &OPF<float>::setDensityEstimationMode,
            py::arg("mode"),
            "Set density estimation mode used in PDF accumulation.")
        .def("get_density_estimation_mode", &OPF<float>::getDensityEstimationMode,
            "Get current density estimation mode.")
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
        .def("create_arcs", &OPF<float>::createArcs,
            py::arg("subgraph"), py::arg("knn"),
            "Build k-NN adjacency lists in-place and set sg.df/bestk and per-node radius. "
            "Mirrors opf_CreateArcs from LibOPF.")
        .def("destroy_arcs", &OPF<float>::destroyArcs,
            py::arg("subgraph"),
            "Clear adjacency lists and plateau adjacency counters. Mirrors opf_DestroyArcs.")
        .def("compute_pdf", &OPF<float>::computePDF,
            py::arg("subgraph"),
            "Compute Gaussian kernel PDF density over the kNN graph and store the normalized "
            "density in each node's dens and pathval fields.  Mirrors opf_PDF from LibOPF. "
            "Requires adjacency lists and sg.df to be set before calling.")
        .def("bestk_min_cut", &OPF<float>::bestkMinCut,
            py::arg("subgraph"), py::arg("kmin"), py::arg("kmax"),
            "Select best k by normalized cut minimization, then rebuild arcs and compute PDF. "
            "Mirrors opf_BestkMinCut from LibOPF.")
        .def("bestk_min_cut_per_kernel", &OPF<float>::bestkMinCutPerKernel,
            py::arg("kernels"), py::arg("kmin"), py::arg("kmax"),
            "Run best-k minimization independently for each KernelSubGraph and return per-kernel results.")
        .def("update_joint_probabilities_from_kernels", &OPF<float>::updateJointProbabilitiesFromKernels,
            py::arg("kernels"), py::arg("accumulator"), py::arg("epsilon") = 1e-12f,
            "Compute ln(dens) kernel probabilities and update central accumulator with replace semantics.")
        .def("apply_joint_probabilities_to_subgraph", &OPF<float>::applyJointProbabilitiesToSubgraph,
            py::arg("subgraph"), py::arg("accumulator"),
            "Materialize central joint probabilities into subgraph dens/pathval.")
        .def("cluster_with_joint_probabilities", &OPF<float>::clusterWithJointProbabilities,
            py::arg("subgraph"), py::arg("accumulator"),
            "Apply central joint probabilities and run standard clustering.")
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

    // Distance functions for float specialization
    m.def("eucl_dist", &opf::distance::euclDist<float>, "Euclidean distance between two float vectors");
    m.def("chi_squared_dist", &opf::distance::chiSquaredDist<float>, "Chi-Squared distance between two float vectors");
    m.def("manhattan_dist", &opf::distance::manhattanDist<float>, "Manhattan distance between two float vectors");
    m.def("canberra_dist", &opf::distance::canberraDist<float>, "Canberra distance between two float vectors");
    m.def("squared_chord_dist", &opf::distance::squaredChordDist<float>, "Squared Chord distance between two float vectors");
    m.def("squared_chi_squared_dist", &opf::distance::squaredChiSquaredDist<float>, "Squared Chi-Squared distance between two float vectors");
    m.def("bray_curtis_dist", &opf::distance::brayCurtisDist<float>, "Bray-Curtis distance between two float vectors");

    // Distance functions for double specialization
    m.def("eucl_dist_double", &opf::distance::euclDist<double>, "Euclidean distance between two double vectors");
    m.def("chi_squared_dist_double", &opf::distance::chiSquaredDist<double>, "Chi-Squared distance between two double vectors");
    m.def("manhattan_dist_double", &opf::distance::manhattanDist<double>, "Manhattan distance between two double vectors");
    m.def("canberra_dist_double", &opf::distance::canberraDist<double>, "Canberra distance between two double vectors");
    m.def("squared_chord_dist_double", &opf::distance::squaredChordDist<double>, "Squared Chord distance between two double vectors");
    m.def("squared_chi_squared_dist_double", &opf::distance::squaredChiSquaredDist<double>, "Squared Chi-Squared distance between two double vectors");
    m.def("bray_curtis_dist_double", &opf::distance::brayCurtisDist<double>, "Bray-Curtis distance between two double vectors");

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

    // -----------------------------------------------------------------------
    // KernelNode<float> — proxy node returned by KernelSubGraph::get_node()
    // -----------------------------------------------------------------------
    py::class_<opf::KernelNode<float>>(m, "KernelNode")
        .def_property("pathval",
            &opf::KernelNode<float>::getPathval, &opf::KernelNode<float>::setPathval)
        .def_property("dens",
            &opf::KernelNode<float>::getDens, &opf::KernelNode<float>::setDens)
        .def_property("radius",
            &opf::KernelNode<float>::getRadius, &opf::KernelNode<float>::setRadius)
        .def_property("label",
            &opf::KernelNode<float>::getLabel, &opf::KernelNode<float>::setLabel)
        .def_property("root",
            &opf::KernelNode<float>::getRoot, &opf::KernelNode<float>::setRoot)
        .def_property("pred",
            &opf::KernelNode<float>::getPred, &opf::KernelNode<float>::setPred)
        .def_property("truelabel",
            &opf::KernelNode<float>::getTruelabel, &opf::KernelNode<float>::setTruelabel)
        .def_property("position",
            &opf::KernelNode<float>::getPosition, &opf::KernelNode<float>::setPosition)
        .def_property("status",
            [](const opf::KernelNode<float>& n) { return static_cast<int>(n.getStatus()); },
            [](opf::KernelNode<float>& n, int v) { n.setStatus(static_cast<uint8_t>(v)); })
        .def_property("relevant",
            [](const opf::KernelNode<float>& n) { return static_cast<int>(n.getRelevant()); },
            [](opf::KernelNode<float>& n, int v) { n.setRelevant(static_cast<uint8_t>(v)); })
        .def_property("nplatadj",
            &opf::KernelNode<float>::getNplatadj, &opf::KernelNode<float>::setNplatadj)
        .def_property("feat",
            [](const opf::KernelNode<float>& n) { return *(n.getFeat()); },
            [](opf::KernelNode<float>& n, const std::vector<float>& v) {
                n.setFeat(std::make_shared<std::vector<float>>(v));
            })
        .def_property("adj",
            [](const opf::KernelNode<float>& n) { return n.getAdj(); },
            [](opf::KernelNode<float>& n, const std::vector<int>& v) {
                n.getAdj() = v;
            })
        .def("add_to_adj", &opf::KernelNode<float>::addToAdj)
        .def("clear_adj",  &opf::KernelNode<float>::clearAdj)
        .def("flush",      &opf::KernelNode<float>::flush,
             "Flush dirty overlay scalars back to the source Node.");

    // -----------------------------------------------------------------------
    // KernelSubGraph<float> — feature-slice decorator over an existing Subgraph
    // -----------------------------------------------------------------------
    py::class_<opf::KernelSubGraph<float>>(m, "KernelSubGraph")
        .def_property("nfeats",
            &opf::KernelSubGraph<float>::getNumFeats, &opf::KernelSubGraph<float>::setNumFeats)
        .def_property("bestk",
            &opf::KernelSubGraph<float>::getBestK, &opf::KernelSubGraph<float>::setBestK)
        .def_property("nlabels",
            &opf::KernelSubGraph<float>::getNumLabels, &opf::KernelSubGraph<float>::setNumLabels)
        .def_property("df",
            &opf::KernelSubGraph<float>::getDf, &opf::KernelSubGraph<float>::setDf)
        .def_property("mindens",
            &opf::KernelSubGraph<float>::getMinDens, &opf::KernelSubGraph<float>::setMinDens)
        .def_property("maxdens",
            &opf::KernelSubGraph<float>::getMaxDens, &opf::KernelSubGraph<float>::setMaxDens)
        .def_property("K",
            &opf::KernelSubGraph<float>::getK, &opf::KernelSubGraph<float>::setK)
        .def_property_readonly("nnodes",     &opf::KernelSubGraph<float>::getNumNodes)
        .def_property_readonly("feat_start", &opf::KernelSubGraph<float>::featStart)
        .def_property_readonly("feat_end",   &opf::KernelSubGraph<float>::featEnd)
        .def_property("kernel_feature_sizes",
            &opf::KernelSubGraph<float>::getKernelFeatureSizes,
            &opf::KernelSubGraph<float>::setKernelFeatureSizes)
        .def_property("kernel_weights",
            &opf::KernelSubGraph<float>::getKernelWeights,
            &opf::KernelSubGraph<float>::setKernelWeights)
        .def("get_node",
            [](opf::KernelSubGraph<float>& ksg, int i) -> opf::KernelNode<float>& {
                return ksg.getNode(i);
            },
            py::return_value_policy::reference_internal)
           .def("compute_log_density_probabilities", &opf::KernelSubGraph<float>::computeLogDensityProbabilities,
               py::arg("epsilon") = 1e-12f,
               "Compute per-node prob=ln(dens), guarded as prob=0 when dens<=epsilon.")
        .def("flush_all",   &opf::KernelSubGraph<float>::flushAll,
             "Flush all dirty node overlays back to the source Subgraph.")
        .def("to_subgraph", &opf::KernelSubGraph<float>::toSubgraph,
             "Deep-copy this kernel as a plain Subgraph (only nfeats features per node).");

    // -----------------------------------------------------------------------
    // split_subgraph_into_kernels — free function
    //
    // IMPORTANT: the source Subgraph must remain alive for the lifetime of
    // every returned KernelSubGraph.  py::keep_alive<0,1>() keeps the source
    // alive as long as the returned list object is alive.  Callers should
    // store the source Subgraph alongside the returned list (e.g., as a tuple).
    // -----------------------------------------------------------------------
    m.def("split_subgraph_into_kernels",
        [](opf::Subgraph<float>& sg, const std::vector<std::pair<int, int>>& slices) {
            return opf::splitSubgraphIntoKernels<float>(sg, slices);
        },
        py::arg("subgraph"), py::arg("slices"),
        "Split a Subgraph into KernelSubGraph objects by explicit (offset, size) slices.\n"
        "Each entry in slices is a (offset, size) pair, producing a kernel covering\n"
        "features [offset, offset+size). The source subgraph must stay alive for the\n"
        "lifetime of the returned kernels.");
}
