#ifndef OPF_OPF_HPP
#define OPF_OPF_HPP

#include "Subgraph.hpp"
#include "Distance.hpp"
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <limits>
#include <cmath>
#include <random>

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
                        float weight = distance::euclDist(*sg.getNode(p).getFeat(), *sg.getNode(q).getFeat());
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
                            float weight = distance::euclDist(*sg.getNode(p).getFeat(), *sg.getNode(q).getFeat());
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
                    float weight = distance::euclDist(*train_node.getFeat(), *sg_test.getNode(i).getFeat());
                    if (weight <= train_node.getRadius()) {
                        sg_test.getNode(i).setLabel(train_node.getLabel());
                        break;
                    }
                }
            }
        }

        float accuracy(Subgraph<T>& sg) {
            int correct = 0;
            for (int i = 0; i < sg.getNumNodes(); ++i) {
                if (sg.getNode(i).getLabel() == sg.getNode(i).getTruelabel()) {
                    correct++;
                }
            }
            return (float)correct / sg.getNumNodes();
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
            float acc = 1.0;
            int initial_size = sg_train.getNumNodes();

            training(sg_train);
            classifying(sg_train, sg_eval);
            acc = accuracy(sg_eval);

            while(acc >= desired_acc) {
                // Mark relevant nodes
                for(int i = 0; i < sg_train.getNumNodes(); ++i) sg_train.getNode(i).setRelevant(0);
                for(int i = 0; i < sg_eval.getNumNodes(); ++i) {
                    if (sg_eval.getNode(i).getLabel() == sg_eval.getNode(i).getTruelabel()) {
                        markNodes(sg_train, sg_eval.getNode(i).getPred());
                    }
                }
                removeIrrelevantNodes(sg_train);
                training(sg_train);
                classifying(sg_train, sg_eval);
                acc = accuracy(sg_eval);
            }

            return (1.0f - (float)sg_train.getNumNodes() / initial_size);
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
    };

} // namespace opf

#endif // OPF_OPF_HPP
