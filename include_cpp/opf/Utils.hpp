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

        std::map<int, int> label_count;
        std::map<int, int> sg1_label_count;
        
        for (int i = 0; i < original.getNumNodes(); ++i) {
            label_count[original.getNode(i).getTruelabel()]++;
        }

        for(auto const& [label, count] : label_count) {
            sg1_label_count[label] = std::max(1, static_cast<int>(percentage_sg1 * count));
        }

        std::vector<int> indices(original.getNumNodes());
        std::iota(indices.begin(), indices.end(), 0);
        
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(indices.begin(), indices.end(), g);

        std::vector<bool> assigned_to_sg1(original.getNumNodes(), false);
        int sg1_total_nodes = 0;

        for (int index : indices) {
            int label = original.getNode(index).getTruelabel();
            if (sg1_label_count[label] > 0) {
                assigned_to_sg1[index] = true;
                sg1_label_count[label]--;
                sg1_total_nodes++;
            }
        }
        
        sg1 = Subgraph<T>(sg1_total_nodes);
        sg2 = Subgraph<T>(original.getNumNodes() - sg1_total_nodes);
        sg1.setNumFeats(original.getNumFeats());
        sg2.setNumFeats(original.getNumFeats());
        sg1.setNumLabels(original.getNumLabels());
        sg2.setNumLabels(original.getNumLabels());

        int sg1_idx = 0;
        int sg2_idx = 0;
        for(int i = 0; i < original.getNumNodes(); ++i) {
            if(assigned_to_sg1[i]) {
                sg1.getNode(sg1_idx++) = original.getNode(i);
            } else {
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
