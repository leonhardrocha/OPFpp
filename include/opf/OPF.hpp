#ifndef OPF_OPF_HPP
#define OPF_OPF_HPP

#include "Subgraph.hpp"
#include "Distance.hpp"
#include "ComponentTree.hpp"
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <cmath>
#include <random>
#include <numeric>

namespace opf {

    template<typename T>
    class OPF {
    private:
        void mstPrototypes(Subgraph<T>& sg) {
            std::vector<float> pathval(sg.getNumNodes(), std::numeric_limits<float>::max());
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>, std::greater<Elem>> Q;

            for (int p = 0; p < sg.getNumNodes(); ++p) {
                sg.getNode(p).setStatus(0);
            }

            pathval[0] = 0;
            sg.getNode(0).setPred(-1); // NIL
            Q.push({0, 0});

            while (!Q.empty()) {
                int p = Q.top().second;
                Q.pop();

                sg.getNode(p).setPathval(pathval[p]);

                int pred_p = sg.getNode(p).getPred();
                if (pred_p != -1) {
                    if (sg.getNode(p).getTruelabel() != sg.getNode(pred_p).getTruelabel()) {
                        if (sg.getNode(p).getStatus() != 1) {
                            sg.getNode(p).setStatus(1);
                        }
                        if (sg.getNode(pred_p).getStatus() != 1) {
                            sg.getNode(pred_p).setStatus(1);
                        }
                    }
                }

                for (int q = 0; q < sg.getNumNodes(); ++q) {
                    if (p != q) {
                        float weight = distance::euclDist<T>(*sg.getNode(p).getFeat(), *sg.getNode(q).getFeat());
                        if (weight < pathval[q]) {
                            sg.getNode(q).setPred(p);
                            pathval[q] = weight;
                            Q.push({weight, q});
                        }
                    }
                }
            }
        }
        
        void swapErrorsByNonPrototypes(Subgraph<T>& sg_train, Subgraph<T>& sg_eval) {
            std::vector<int> non_prototypes;
            for(int i = 0; i < sg_train.getNumNodes(); ++i) {
                if(sg_train.getNode(i).getPred() != -1) {
                    non_prototypes.push_back(i);
                }
            }

            std::vector<int> errors;
            for(int i = 0; i < sg_eval.getNumNodes(); ++i) {
                if(sg_eval.getNode(i).getLabel() != sg_eval.getNode(i).getTruelabel()) {
                    errors.push_back(i);
                }
            }
            
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(non_prototypes.begin(), non_prototypes.end(), g);

            int n_swaps = std::min(non_prototypes.size(), errors.size());
            for(int i = 0; i < n_swaps; ++i) {
                std::swap(sg_train.getNode(non_prototypes[i]), sg_eval.getNode(errors[i]));
            }
        }

        void markNodes(Subgraph<T>& sg, int i) {
            while(sg.getNode(i).getPred() != -1){
                sg.getNode(i).setRelevant(1);
                i = sg.getNode(i).getPred();
            }
            sg.getNode(i).setRelevant(1);
        }

        // Classify eval nodes and mark the winning training node (conqueror) and its
        // predecessor path as relevant.  Mirrors opf_OPFClassifyingAndMarkNodes.
        void classifyingAndMarkNodes(Subgraph<T>& sg_train, Subgraph<T>& sg_eval) {
            const auto& ordered_nodes = sg_train.getOrderedListOfNodes();
            for (int i = 0; i < sg_eval.getNumNodes(); ++i) {
                float min_cost = std::numeric_limits<float>::max();
                int conqueror = -1;

                for (int node_idx : ordered_nodes) {
                    const auto& train_node = sg_train.getNode(node_idx);
                    if (min_cost <= train_node.getPathval()) break;

                    float weight = distance::euclDist<T>(*train_node.getFeat(), *sg_eval.getNode(i).getFeat());
                    float cost = std::max(train_node.getPathval(), weight);

                    if (cost < min_cost) {
                        min_cost = cost;
                        sg_eval.getNode(i).setLabel(train_node.getLabel());
                        conqueror = node_idx;
                    }
                }

                if (conqueror != -1) {
                    markNodes(sg_train, conqueror);
                }
            }
        }

        void removeIrrelevantNodes(Subgraph<T>& sg) {
            std::vector<Node<T>> relevant_nodes;
            for(int i = 0; i < sg.getNumNodes(); ++i) {
                if (sg.getNode(i).getRelevant()) {
                    relevant_nodes.push_back(sg.getNode(i));
                }
            }
            sg.setNodes(relevant_nodes);
        }


    public:
        void training(Subgraph<T>& sg) {
            mstPrototypes(sg);

            std::vector<float> pathval(sg.getNumNodes(), std::numeric_limits<float>::max());
            
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>, std::greater<Elem>> Q;

            for (int p = 0; p < sg.getNumNodes(); ++p) {
                if (sg.getNode(p).getStatus() == 1) { // opf_PROTOTYPE
                    sg.getNode(p).setPred(-1); // NIL
                    pathval[p] = 0;
                    sg.getNode(p).setLabel(sg.getNode(p).getTruelabel());
                    Q.push({0, p});
                }
            }
            
            sg.clearOrderedListOfNodes();
            while (!Q.empty()) {
                int p = Q.top().second;
                Q.pop();

                sg.getNode(p).setPathval(pathval[p]);
                sg.addOrderedNode(p);

                for (int q = 0; q < sg.getNumNodes(); ++q) {
                    if (p != q) {
                        if (pathval[p] < pathval[q]) {
                            float weight = distance::euclDist<T>(*sg.getNode(p).getFeat(), *sg.getNode(q).getFeat());
                            float tmp = std::max(pathval[p], weight);
                            if (tmp < pathval[q]) {
                                sg.getNode(q).setPred(p);
                                sg.getNode(q).setLabel(sg.getNode(p).getLabel());
                                pathval[q] = tmp;
                                Q.push({tmp, q});
                            }
                        }
                    }
                }
            }
        }

        void classifying(const Subgraph<T>& sg_train, Subgraph<T>& sg_test) {
            const auto& ordered_nodes = sg_train.getOrderedListOfNodes();

            for (int i = 0; i < sg_test.getNumNodes(); ++i) {
                float min_cost = std::numeric_limits<float>::max();
                int final_label = -1;

                for (int node_idx : ordered_nodes) {
                    const auto& train_node = sg_train.getNode(node_idx);
                    
                    if (min_cost <= train_node.getPathval()) {
                        break;
                    }

                    float weight = distance::euclDist(*train_node.getFeat(), *sg_test.getNode(i).getFeat());
                    float cost = std::max(train_node.getPathval(), weight);

                    if (cost < min_cost) {
                        min_cost = cost;
                        final_label = train_node.getLabel();
                    }
                }
                sg_test.getNode(i).setLabel(final_label);
            }
        }

        void knnClassifying(const Subgraph<T>& sg_train, Subgraph<T>& sg_test) {
            const auto& ordered_nodes = sg_train.getOrderedListOfNodes();
            for (int i = 0; i < sg_test.getNumNodes(); ++i) {
                for (int node_idx : ordered_nodes) {
                    const auto& train_node = sg_train.getNode(node_idx);
                    float weight = distance::euclDist<T>(*train_node.getFeat(), *sg_test.getNode(i).getFeat());
                    if (weight <= train_node.getRadius()) {
                        sg_test.getNode(i).setLabel(train_node.getLabel());
                        break;
                    }
                }
            }
        }

        float accuracy(Subgraph<T>& sg) {
            // Compute accuracy using error matrix (matching C implementation)
            int nlabels = sg.getNumLabels();
            int nnodes = sg.getNumNodes();
            
            // Create error matrix: error_matrix[label][0] = fp rate, [1] = fn rate
            std::vector<std::vector<float>> error_matrix(nlabels + 1, std::vector<float>(2, 0.0f));
            
            // Count samples per true label
            std::vector<int> nclass(nlabels + 1, 0);
            for (int i = 0; i < nnodes; ++i) {
                int true_label = sg.getNode(i).getTruelabel();
                if (true_label >= 0 && true_label <= nlabels) {
                    nclass[true_label]++;
                }
            }
            
            // Count errors
            for (int i = 0; i < nnodes; ++i) {
                int true_label = sg.getNode(i).getTruelabel();
                int pred_label = sg.getNode(i).getLabel();
                
                if (true_label != pred_label) {
                    if (true_label >= 0 && true_label <= nlabels) {
                        error_matrix[true_label][1]++;  // false negative
                    }
                    if (pred_label >= 0 && pred_label <= nlabels) {
                        error_matrix[pred_label][0]++;  // false positive
                    }
                }
            }
            
            // Normalize errors by class size
            int nlabels_with_samples = 0;
            for (int i = 1; i <= nlabels; ++i) {
                if (nclass[i] != 0) {
                    error_matrix[i][1] /= (float)nclass[i];  // normalize FN by true positives
                    int negatives = nnodes - nclass[i];
                    // Guard: if this label owns ALL nodes there are no negatives,
                    // so FP rate is defined as 0 (nothing could be falsely flagged).
                    error_matrix[i][0] = (negatives > 0)
                        ? error_matrix[i][0] / (float)negatives
                        : 0.0f;
                    nlabels_with_samples++;
                }
            }
            
            // Compute overall accuracy
            float error = 0.0f;
            for (int i = 1; i <= nlabels; ++i) {
                if (nclass[i] != 0) {
                    error += (error_matrix[i][0] + error_matrix[i][1]);
                }
            }
            
            return (nlabels_with_samples > 0) ? (1.0f - (error / (2.0f * nlabels_with_samples))) : 0.0f;
        }

        void normalize(Subgraph<T>& sg) {
            std::vector<float> mean(sg.getNumFeats(), 0.0f);
            std::vector<float> std_dev(sg.getNumFeats(), 0.0f);

            for (int i = 0; i < sg.getNumFeats(); ++i) {
                for (int j = 0; j < sg.getNumNodes(); ++j) {
                    mean[i] += (*sg.getNode(j).getFeat())[i];
                }
                mean[i] /= sg.getNumNodes();
            }

            for (int i = 0; i < sg.getNumFeats(); ++i) {
                for (int j = 0; j < sg.getNumNodes(); ++j) {
                    std_dev[i] += std::pow((*sg.getNode(j).getFeat())[i] - mean[i], 2);
                }
                std_dev[i] = std::sqrt(std_dev[i] / sg.getNumNodes());
                if (std_dev[i] == 0.0) {
                    std_dev[i] = 1.0;
                }
            }

            for (int i = 0; i < sg.getNumNodes(); ++i) {
                for (int j = 0; j < sg.getNumFeats(); ++j) {
                    (*sg.getNode(i).getFeat())[j] = ((*sg.getNode(i).getFeat())[j] - mean[j]) / std_dev[j];
                }
            }
        }
        
        void clustering(Subgraph<T>& sg) {
            std::vector<float> pathval(sg.getNumNodes());
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>> Q;

            for (int p = 0; p < sg.getNumNodes(); ++p) {
                pathval[p] = sg.getNode(p).getPathval();
                sg.getNode(p).setPred(-1); // NIL
                sg.getNode(p).setRoot(p);
                Q.push({pathval[p], p});
            }

            int l = 0;
            sg.clearOrderedListOfNodes();
            while (!Q.empty()) {
                int p = Q.top().second;
                Q.pop();
                sg.addOrderedNode(p);

                if (sg.getNode(p).getPred() == -1) {
                    pathval[p] = sg.getNode(p).getDens();
                    sg.getNode(p).setLabel(l++);
                }

                sg.getNode(p).setPathval(pathval[p]);

                for (int q : sg.getNode(p).getAdj()) {
                    if (q < 0 || q >= sg.getNumNodes()) continue; // bounds check
                    float tmp = std::min(pathval[p], sg.getNode(q).getDens());
                    if (tmp > pathval[q]) {
                        pathval[q] = tmp;
                        sg.getNode(q).setPred(p);
                        sg.getNode(q).setRoot(sg.getNode(p).getRoot());
                        sg.getNode(q).setLabel(sg.getNode(p).getLabel());
                        Q.push({pathval[q], q});
                    }
                }
            }
            sg.setNumLabels(l);
        }

        void learning(Subgraph<T>& sg_train, Subgraph<T>& sg_eval, int n_iterations = 10) {
            float acc = 0.0, prev_acc = -1.0, max_acc = 0.0;
            Subgraph<T> best_sg;

            for(int i = 0; i < n_iterations; ++i) {
                prev_acc = acc;
                
                training(sg_train);
                classifying(sg_train, sg_eval);
                acc = accuracy(sg_eval);

                if (acc > max_acc) {
                    max_acc = acc;
                    best_sg = sg_train;
                }

                if (std::abs(acc - prev_acc) < 0.0001) {
                    break;
                }

                swapErrorsByNonPrototypes(sg_train, sg_eval);
            }
            sg_train = best_sg;
        }

        float pruning(Subgraph<T>& sg_train, Subgraph<T>& sg_eval, float desired_acc) {
            const int max_iterations = 100;
            const int initial_size = sg_train.getNumNodes();
            int t = 1;

            // Initial train + evaluate
            training(sg_train);
            classifying(sg_train, sg_eval);
            float current_acc = accuracy(sg_eval);
            float old_acc = current_acc;

            // Loop while accuracy change stays within the tolerance.
            // Mirrors opf_OPFPruning: desiredAcc is a *tolerance*, not a floor.
            while (t <= max_iterations && std::fabs(current_acc - old_acc) <= desired_acc) {
                old_acc = current_acc;

                // Reset relevant flag and predecessor for all training nodes
                for (int i = 0; i < sg_train.getNumNodes(); ++i) {
                    sg_train.getNode(i).setRelevant(0);
                    sg_train.getNode(i).setPred(-1);
                }

                training(sg_train);
                classifyingAndMarkNodes(sg_train, sg_eval);
                removeIrrelevantNodes(sg_train);

                // Re-train on pruned set, then re-evaluate
                training(sg_train);
                classifying(sg_train, sg_eval);
                current_acc = accuracy(sg_eval);
                ++t;
            }

            return 1.0f - (float)sg_train.getNumNodes() / (float)initial_size;
        }
        
        Subgraph<T> semiSupervisedLearning(Subgraph<T>& sg_labeled, Subgraph<T>& sg_unlabeled, Subgraph<T>* sg_eval) {
            Subgraph<T> merged = Subgraph<T>::merge(sg_labeled, sg_unlabeled);
            
            if (sg_eval) {
                learning(merged, *sg_eval);
            }

            mstPrototypes(merged);
            training(merged);
            
            return merged;
        }

        // ---- Arc management -------------------------------------------------

        /// Build a k-NN adjacency graph.  Sets sg.df (max arc weight among all
        /// knn neighbors) and each node's radius to its farthest knn neighbour.
        /// Mirrors opf_CreateArcs from LibOPF.
        void createArcs(Subgraph<T>& sg, int knn) {
            const int n = sg.getNumNodes();
            float df = 0.0f;

            for (int i = 0; i < n; ++i) {
                sg.getNode(i).clearAdj();

                // Insertion-sort the knn nearest neighbours into (d, idx) arrays
                std::vector<float> d(knn + 1, std::numeric_limits<float>::max());
                std::vector<int>   nn(knn + 1, -1);

                for (int j = 0; j < n; ++j) {
                    if (j == i) continue;
                    d[knn]  = distance::euclDist<T>(*sg.getNode(i).getFeat(), *sg.getNode(j).getFeat());
                    nn[knn] = j;
                    int k = knn;
                    while (k > 0 && d[k] < d[k - 1]) {
                        std::swap(d[k], d[k - 1]);
                        std::swap(nn[k], nn[k - 1]);
                        --k;
                    }
                }

                float radius = 0.0f;
                for (int l = 0; l < knn; ++l) {
                    if (d[l] < std::numeric_limits<float>::max()) {
                        if (d[l] > df) df = d[l];
                        if (d[l] > radius) radius = d[l];
                        sg.getNode(i).addToAdj(nn[l]);
                    }
                }
                sg.getNode(i).setRadius(radius);
            }

            sg.setDf(df < 1e-5f ? 1.0f : df);
            sg.setBestK(knn);
        }

        /// Clear all adjacency lists (mirrors opf_DestroyArcs).
        void destroyArcs(Subgraph<T>& sg) {
            for (int i = 0; i < sg.getNumNodes(); ++i) {
                sg.getNode(i).clearAdj();
                sg.getNode(i).setNplatadj(0);
            }
        }

        // ---- PDF density computation ----------------------------------------

        /// Compute the Gaussian kernel PDF over the kNN graph and store the
        /// normalized density in each node.  Mirrors opf_PDF from LibOPF.
        void computePDF(Subgraph<T>& sg) {
            const int n = sg.getNumNodes();
            const float K = 2.0f * sg.getDf() / 9.0f;
            sg.setK(K);

            std::vector<float> value(n);
            float mindens =  std::numeric_limits<float>::max();
            float maxdens = -std::numeric_limits<float>::max();

            for (int i = 0; i < n; ++i) {
                float sum = 0.0f;
                int   nelems = 1;
                for (int q : sg.getNode(i).getAdj()) {
                    float dist = distance::euclDist<T>(*sg.getNode(i).getFeat(), *sg.getNode(q).getFeat());
                    sum += std::exp(-dist / K);
                    ++nelems;
                }
                value[i] = sum / static_cast<float>(nelems);
                if (value[i] < mindens) mindens = value[i];
                if (value[i] > maxdens) maxdens = value[i];
            }

            sg.setMinDens(mindens);
            sg.setMaxDens(maxdens);

            if (mindens == maxdens) {
                for (int i = 0; i < n; ++i) {
                    sg.getNode(i).setDens(opf_MAXDENS);
                    sg.getNode(i).setPathval(opf_MAXDENS - 1.0f);
                }
            } else {
                for (int i = 0; i < n; ++i) {
                    float dens = (opf_MAXDENS - 1.0f) * (value[i] - mindens) / (maxdens - mindens) + 1.0f;
                    sg.getNode(i).setDens(dens);
                    sg.getNode(i).setPathval(dens - 1.0f);
                }
            }
        }

        // ---- Maxima suppression ----------------------------------------------

        /// Suppress density maxima below height H (pathval = max(dens-H, 0)).
        void elimMaxBelowH(Subgraph<T>& sg, float H) {
            if (H <= 0.0f) return;
            for (int i = 0; i < sg.getNumNodes(); ++i)
                sg.getNode(i).setPathval(std::max(sg.getNode(i).getDens() - H, 0.0f));
        }

        /// Suppress density maxima whose area (component size) is below A.
        void elimMaxBelowArea(Subgraph<T>& sg, int A) {
            const int n = sg.getNumNodes();
            std::vector<std::vector<int>> adj(n);
            for (int i = 0; i < n; ++i)
                adj[i] = sg.getNode(i).getAdj();

            std::vector<int> densInt(n);
            for (int i = 0; i < n; ++i)
                densInt[i] = static_cast<int>(sg.getNode(i).getDens());

            auto area = sgAreaOpen<T>(adj, densInt, n, A);
            for (int i = 0; i < n; ++i)
                sg.getNode(i).setPathval(std::max(area[i] - 1, 0));
        }

        /// Suppress density maxima whose volume is below V.
        void elimMaxBelowVolume(Subgraph<T>& sg, int V) {
            const int n = sg.getNumNodes();
            std::vector<std::vector<int>> adj(n);
            for (int i = 0; i < n; ++i)
                adj[i] = sg.getNode(i).getAdj();

            std::vector<int> densInt(n);
            for (int i = 0; i < n; ++i)
                densInt[i] = static_cast<int>(sg.getNode(i).getDens());

            auto vol = sgVolumeOpen<T>(adj, densInt, n, V);
            for (int i = 0; i < n; ++i)
                sg.getNode(i).setPathval(std::max(vol[i] - 1, 0));
        }

        // ---- Normalized cut -------------------------------------------------

        /// Compute the normalised cut value over the full adjacency graph.
        /// Mirrors opf_NormalizedCut from LibOPF.
        float normalizedCut(Subgraph<T>& sg) {
            const int n       = sg.getNumNodes();
            const int nlabels = sg.getNumLabels();
            std::vector<float> acumIC(nlabels, 0.0f);
            std::vector<float> acumEC(nlabels, 0.0f);

            for (int p = 0; p < n; ++p) {
                for (int q : sg.getNode(p).getAdj()) {
                    float dist = distance::euclDist<T>(*sg.getNode(p).getFeat(), *sg.getNode(q).getFeat());
                    if (dist > 0.0f) {
                        int lp = sg.getNode(p).getLabel();
                        int lq = sg.getNode(q).getLabel();
                        if (lp == lq)
                            acumIC[lp] += 1.0f / dist;
                        else {
                            acumEC[lp] += 1.0f / dist;
                            acumEC[lq] += 1.0f / dist;
                        }
                    }
                }
            }

            float ncut = 0.0f;
            for (int l = 0; l < nlabels; ++l) {
                float denom = acumIC[l] + acumEC[l];
                if (denom > 0.0f)
                    ncut += acumEC[l] / denom;
            }
            return ncut;
        }

        // ---- BestkMinCut (unsupervised best-k selection) --------------------

        /// Find the best k in [kmin, kmax] by minimising the normalised cut.
        /// After the call sg is left with arcs for the best k and PDF computed.
        /// Mirrors opf_BestkMinCut from LibOPF.
        void bestkMinCut(Subgraph<T>& sg, int kmin, int kmax) {
            // For each k build max-dist array (outer kmax pass)
            std::vector<float> maxdists = createArcs2(sg, kmax);

            int   bestk   = kmax;
            float mincut  = std::numeric_limits<float>::max();

            for (int k = kmin; k <= kmax && mincut != 0.0f; ++k) {
                sg.setDf(maxdists[k - 1]);
                sg.setBestK(k);

                pdfToKmax(sg);
                clusteringToKmax(sg);
                float nc = normalizedCutToKmax(sg);

                if (nc < mincut) {
                    mincut = nc;
                    bestk  = k;
                }
            }

            destroyArcs(sg);
            sg.setBestK(bestk);
            createArcs(sg, bestk);
            computePDF(sg);
        }

    private:
        // ---- Helpers for bestkMinCut ----------------------------------------

        static constexpr float opf_MAXDENS  = 1000.0f;
        static constexpr float opf_MAXARCW  = 100000.0f;

        /// Build the kmax-NN graph and return the max arc distance at each k=1..kmax.
        /// Mirrors opf_CreateArcs2: sg adjacency is set to the full kmax neighbors.
        std::vector<float> createArcs2(Subgraph<T>& sg, int kmax) {
            const int n = sg.getNumNodes();
            std::vector<float> maxdists(kmax, 0.0f);  // maxdists[k-1] = max df at k

            for (int i = 0; i < n; ++i) {
                sg.getNode(i).clearAdj();
                sg.getNode(i).setNplatadj(0);

                std::vector<float> d(kmax + 1, std::numeric_limits<float>::max());
                std::vector<int>   nn(kmax + 1, -1);

                for (int j = 0; j < n; ++j) {
                    if (j == i) continue;
                    d[kmax]  = distance::euclDist<T>(*sg.getNode(i).getFeat(), *sg.getNode(j).getFeat());
                    nn[kmax] = j;
                    int pos = kmax;
                    while (pos > 0 && d[pos] < d[pos - 1]) {
                        std::swap(d[pos], d[pos - 1]);
                        std::swap(nn[pos], nn[pos - 1]);
                        --pos;
                    }
                }

                for (int l = 0; l < kmax; ++l) {
                    if (d[l] < std::numeric_limits<float>::max()) {
                        if (d[l] > maxdists[l]) maxdists[l] = d[l];
                        sg.getNode(i).addToAdj(nn[l]);
                    }
                }
            }

            // maxdists[k-1] should be the max over all 1..k neighbors, not just k-th
            for (int k = 1; k < kmax; ++k)
                if (maxdists[k] < maxdists[k - 1])
                    maxdists[k] = maxdists[k - 1];

            return maxdists;
        }

        /// PDF computation limited to the first bestk neighbors in the adjacency
        /// list (plateau neighbors excluded).  Mirrors opf_PDFtoKmax.
        void pdfToKmax(Subgraph<T>& sg) {
            const int   n    = sg.getNumNodes();
            const int   kmax = sg.getBestK();
            const float K    = 2.0f * sg.getDf() / 9.0f;
            sg.setK(K);

            std::vector<float> value(n);
            float mindens =  std::numeric_limits<float>::max();
            float maxdens = -std::numeric_limits<float>::max();

            for (int i = 0; i < n; ++i) {
                float sum   = 0.0f;
                int   nelems = 1;
                const auto& adjList = sg.getNode(i).getAdj();
                int k = 0;
                for (int q : adjList) {
                    if (k >= kmax) break;
                    float dist = distance::euclDist<T>(*sg.getNode(i).getFeat(), *sg.getNode(q).getFeat());
                    sum += std::exp(-dist / K);
                    ++nelems;
                    ++k;
                }
                value[i] = sum / static_cast<float>(nelems);
                if (value[i] < mindens) mindens = value[i];
                if (value[i] > maxdens) maxdens = value[i];
            }

            sg.setMinDens(mindens);
            sg.setMaxDens(maxdens);

            if (mindens == maxdens) {
                for (int i = 0; i < n; ++i) {
                    sg.getNode(i).setDens(opf_MAXDENS);
                    sg.getNode(i).setPathval(opf_MAXDENS - 1.0f);
                }
            } else {
                for (int i = 0; i < n; ++i) {
                    float dens = (opf_MAXDENS - 1.0f) * (value[i] - mindens) / (maxdens - mindens) + 1.0f;
                    sg.getNode(i).setDens(dens);
                    sg.getNode(i).setPathval(dens - 1.0f);
                }
            }
        }

        /// OPF clustering limited to the first bestk neighbors.
        /// Mirrors opf_OPFClusteringToKmax.
        void clusteringToKmax(Subgraph<T>& sg) {
            const int n    = sg.getNumNodes();
            const int kmax = sg.getBestK();

            std::vector<float> pathval(n);
            using Elem = std::pair<float, int>;
            std::priority_queue<Elem, std::vector<Elem>> Q;  // max-heap

            for (int p = 0; p < n; ++p) {
                pathval[p] = sg.getNode(p).getPathval();
                sg.getNode(p).setPred(NIL);
                sg.getNode(p).setRoot(p);
                Q.push({pathval[p], p});
            }

            int l = 0;
            sg.clearOrderedListOfNodes();
            while (!Q.empty()) {
                auto [pv, p] = Q.top(); Q.pop();
                sg.addOrderedNode(p);

                if (sg.getNode(p).getPred() == NIL) {
                    pathval[p] = sg.getNode(p).getDens();
                    sg.getNode(p).setLabel(l++);
                }
                sg.getNode(p).setPathval(pathval[p]);

                const auto& adjList = sg.getNode(p).getAdj();
                const int nadj = kmax + sg.getNode(p).getNplatadj();
                int k = 0;
                for (int q : adjList) {
                    if (k >= nadj) break;
                    if (q < 0 || q >= n) { ++k; continue; }
                    float tmp = std::min(pathval[p], sg.getNode(q).getDens());
                    if (tmp > pathval[q]) {
                        pathval[q] = tmp;
                        sg.getNode(q).setPred(p);
                        sg.getNode(q).setRoot(sg.getNode(p).getRoot());
                        sg.getNode(q).setLabel(sg.getNode(p).getLabel());
                        Q.push({pathval[q], q});
                    }
                    ++k;
                }
            }
            sg.setNumLabels(l);
        }

        /// Normalised cut limited to the first bestk neighbors.
        /// Mirrors opf_NormalizedCutToKmax.
        float normalizedCutToKmax(Subgraph<T>& sg) {
            const int n      = sg.getNumNodes();
            const int nlabels = sg.getNumLabels();
            const int kmax   = sg.getBestK();
            std::vector<float> acumIC(nlabels, 0.0f);
            std::vector<float> acumEC(nlabels, 0.0f);

            for (int p = 0; p < n; ++p) {
                const auto& adjList = sg.getNode(p).getAdj();
                const int nadj = kmax + sg.getNode(p).getNplatadj();
                int k = 0;
                for (int q : adjList) {
                    if (k >= nadj) break;
                    float dist = distance::euclDist<T>(*sg.getNode(p).getFeat(), *sg.getNode(q).getFeat());
                    if (dist > 0.0f) {
                        int lp = sg.getNode(p).getLabel();
                        int lq = sg.getNode(q).getLabel();
                        if (lp == lq)
                            acumIC[lp] += 1.0f / dist;
                        else {
                            acumEC[lp] += 1.0f / dist;
                            acumEC[lq] += 1.0f / dist;
                        }
                    }
                    ++k;
                }
            }

            float ncut = 0.0f;
            for (int l = 0; l < nlabels; ++l) {
                float denom = acumIC[l] + acumEC[l];
                if (denom > 0.0f) ncut += acumEC[l] / denom;
            }
            return ncut;
        }
    };

} // namespace opf

#endif // OPF_OPF_HPP
