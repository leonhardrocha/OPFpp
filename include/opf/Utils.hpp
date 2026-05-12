#ifndef OPF_UTILS_HPP
#define OPF_UTILS_HPP

#include "Subgraph.hpp"
#include <vector>
#include <map>
#include <algorithm>
#include <random>
#include <numeric>

namespace opf {
    template<typename T>
    void split(Subgraph<T>& original, Subgraph<T>& sg1, Subgraph<T>& sg2, float percentage_sg1) {
        if (percentage_sg1 < 0.0 || percentage_sg1 > 1.0) {
            throw std::invalid_argument("Percentage must be between 0.0 and 1.0");
        }

        // Count nodes per label
        std::map<int, int> label_count;
        for (int i = 0; i < original.getNumNodes(); ++i) {
            auto node = original.getNode(i);
            node.setStatus(0); // Reset status for all nodes
            label_count[node.getTruelabel()]++;
        }

        // Calculate how many nodes from each label should go to sg1
        std::map<int, int> nelems;
        for (auto const& [label, count] : label_count) {
            nelems[label] = std::max(1, static_cast<int>(percentage_sg1 * count));
        }

        // Calculate total nodes for sg1
        int sg1_total_nodes = 0;
        for (auto const& [label, count] : nelems) {
            sg1_total_nodes += count;
        }

        // Create subgraphs with correct sizes
        sg1 = Subgraph<T>(sg1_total_nodes);
        sg2 = Subgraph<T>(original.getNumNodes() - sg1_total_nodes);
        sg1.setNumFeats(original.getNumFeats());
        sg2.setNumFeats(original.getNumFeats());
        sg1.setNumLabels(original.getNumLabels());
        sg2.setNumLabels(original.getNumLabels());

        // Track which nodes have been assigned
        std::vector<int> statuses(original.getNumNodes(), 0);
        
        // Randomly assign nodes to sg1 respecting per-label quotas
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, original.getNumNodes() - 1);
        
        int sg1_idx = 0;
        int remaining = sg1_total_nodes;
        
        while (remaining > 0) {
            int i = dis(gen);
            if (statuses[i] == 0) {  // Not yet assigned
                int label = original.getNode(i).getTruelabel();
                if (nelems[label] > 0) {
                    sg1.getNode(sg1_idx++) = original.getNode(i);
                    nelems[label]--;
                    statuses[i] = 1;  // Mark as assigned to sg1
                    remaining--;
                }
            }
        }

        // Assign remaining nodes to sg2
        int sg2_idx = 0;
        for (int i = 0; i < original.getNumNodes(); ++i) {
            if (statuses[i] == 0) {  // Not assigned to sg1
                sg2.getNode(sg2_idx++) = original.getNode(i);
            }
        }
    }

    template<typename T>
    std::vector<Subgraph<T>> kFold(Subgraph<T>& original, int k) {
        if (k <= 0) {
            throw std::invalid_argument("k must be a positive integer.");
        }

        std::vector<Subgraph<T>> folds(k);
        std::map<int, std::vector<int>> nodes_by_label;
        
        for(int i = 0; i < original.getNumNodes(); ++i) {
            nodes_by_label[original.getNode(i).getTruelabel()].push_back(i);
        }

        for(auto const& [label, nodes] : nodes_by_label) {
            int fold_idx = 0;
            for(int node_idx : nodes) {
                folds[fold_idx].addNode(original.getNode(node_idx));
                fold_idx = (fold_idx + 1) % k;
            }
        }
        
        for(int i = 0; i < k; ++i) {
            folds[i].setNumFeats(original.getNumFeats());
            folds[i].setNumLabels(original.getNumLabels());
        }

        return folds;
    }

}

#endif // OPF_UTILS_HPP
