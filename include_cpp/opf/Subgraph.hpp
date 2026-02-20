#ifndef OPF_SUBGRAPH_HPP
#define OPF_SUBGRAPH_HPP

#include "Node.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <numeric>

namespace opf {
    template<typename T>
    class Subgraph {
    public:
        // Default constructor
        Subgraph() = default;

        // Constructor with size
        Subgraph(int n_nodes) : nodes(n_nodes), ordered_list_of_nodes(n_nodes) {}

        // Getters
        int getNumNodes() const { return nodes.size(); }
        int getNumFeats() const { return nfeats; }
        int getBestK() const { return bestk; }
        int getNumLabels() const { return nlabels; }
        float getDf() const { return df; }
        float getMinDens() const { return mindens; }
        float getMaxDens() const { return maxdens; }
        float getK() const { return K; }
        const std::vector<Node<T>>& getNodes() const { return nodes; }
        Node<T>& getNode(int index) { return nodes[index]; }
        const Node<T>& getNode(int index) const { return nodes[index]; }
        const std::vector<int>& getOrderedListOfNodes() const { return ordered_list_of_nodes; }

        // Setters
        void setNumFeats(int val) { nfeats = val; }
        void setBestK(int val) { bestk = val; }
        void setNumLabels(int val) { nlabels = val; }
        void setDf(float val) { df = val; }
        void setMinDens(float val) { mindens = val; }
        void setMaxDens(float val) { maxdens = val; }
        void setK(float val) { K = val; }
        void setNodes(const std::vector<Node<T>>& new_nodes) { nodes = new_nodes; }


        void addNode(const Node<T>& node) {
            nodes.push_back(node);
        }

        void addOrderedNode(int node_index) {
            ordered_list_of_nodes.push_back(node_index);
        }
        
        void clearOrderedListOfNodes() {
            ordered_list_of_nodes.clear();
        }

        void writeModel(const std::string& filename) const {
            std::ofstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for writing: " + filename);
            }

            int nnodes = getNumNodes();
            file.write(reinterpret_cast<const char*>(&nnodes), sizeof(int));
            file.write(reinterpret_cast<const char*>(&nlabels), sizeof(int));
            file.write(reinterpret_cast<const char*>(&nfeats), sizeof(int));
            file.write(reinterpret_cast<const char*>(&df), sizeof(float));
            file.write(reinterpret_cast<const char*>(&K), sizeof(float));
            file.write(reinterpret_cast<const char*>(&mindens), sizeof(float));
            file.write(reinterpret_cast<const char*>(&maxdens), sizeof(float));

            for (const auto& node : nodes) {
                int position = node.getPosition();
                int truelabel = node.getTruelabel();
                int pred = node.getPred();
                int label = node.getLabel();
                float pathval = node.getPathval();
                float radius = node.getRadius();

                file.write(reinterpret_cast<const char*>(&position), sizeof(int));
                file.write(reinterpret_cast<const char*>(&truelabel), sizeof(int));
                file.write(reinterpret_cast<const char*>(&pred), sizeof(int));
                file.write(reinterpret_cast<const char*>(&label), sizeof(int));
                file.write(reinterpret_cast<const char*>(&pathval), sizeof(float));
                file.write(reinterpret_cast<const char*>(&radius), sizeof(float));
                file.write(reinterpret_cast<const char*>(node.getFeat()->data()), nfeats * sizeof(T));
            }

            file.write(reinterpret_cast<const char*>(ordered_list_of_nodes.data()), ordered_list_of_nodes.size() * sizeof(int));
        }

        static Subgraph<T> readModel(const std::string& filename) {
            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for reading: " + filename);
            }
            
            int nnodes, nlabels, nfeats;
            float df, K, mindens, maxdens;

            file.read(reinterpret_cast<char*>(&nnodes), sizeof(int));
            file.read(reinterpret_cast<char*>(&nlabels), sizeof(int));
            file.read(reinterpret_cast<char*>(&nfeats), sizeof(int));
            file.read(reinterpret_cast<char*>(&df), sizeof(float));
            file.read(reinterpret_cast<char*>(&K), sizeof(float));
            file.read(reinterpret_cast<char*>(&mindens), sizeof(float));
            file.read(reinterpret_cast<char*>(&maxdens), sizeof(float));
            
            Subgraph<T> sg(nnodes);
            sg.setNumLabels(nlabels);
            sg.setNumFeats(nfeats);
            sg.setDf(df);
            sg.setK(K);
            sg.setMinDens(mindens);
            sg.setMaxDens(maxdens);

            for (int i = 0; i < nnodes; ++i) {
                int position, truelabel, pred, label;
                float pathval, radius;

                file.read(reinterpret_cast<char*>(&position), sizeof(int));
                file.read(reinterpret_cast<char*>(&truelabel), sizeof(int));
                file.read(reinterpret_cast<char*>(&pred), sizeof(int));
                file.read(reinterpret_cast<char*>(&label), sizeof(int));
                file.read(reinterpret_cast<char*>(&pathval), sizeof(float));
                file.read(reinterpret_cast<char*>(&radius), sizeof(float));

                sg.getNode(i).setPosition(position);
                sg.getNode(i).setTruelabel(truelabel);
                sg.getNode(i).setPred(pred);
                sg.getNode(i).setLabel(label);
                sg.getNode(i).setPathval(pathval);
                sg.getNode(i).setRadius(radius);
                
                auto features = std::make_shared<std::vector<T>>(nfeats);
                file.read(reinterpret_cast<char*>(features->data()), nfeats * sizeof(T));
                sg.getNode(i).setFeat(features);
            }
            
            sg.clearOrderedListOfNodes();
            for (int i = 0; i < nnodes; ++i) {
                int node_idx;
                file.read(reinterpret_cast<char*>(&node_idx), sizeof(int));
                sg.addOrderedNode(node_idx);
            }

            return sg;
        }

        static Subgraph<T> merge(const Subgraph<T>& sg1, const Subgraph<T>& sg2) {
            if (sg1.getNumFeats() != sg2.getNumFeats()) {
                throw std::runtime_error("Cannot merge subgraphs with different number of features.");
            }

            Subgraph<T> merged(sg1.getNumNodes() + sg2.getNumNodes());
            merged.setNumFeats(sg1.getNumFeats());
            merged.setNumLabels(std::max(sg1.getNumLabels(), sg2.getNumLabels()));
            
            int i = 0;
            for (const auto& node : sg1.getNodes()) {
                merged.nodes[i++] = node;
            }
            for (const auto& node : sg2.getNodes()) {
                merged.nodes[i++] = node;
            }

            return merged;
        }


    private:
        std::vector<Node<T>> nodes;
        int nfeats = 0;
        int bestk = 0;
        int nlabels = 0;
        float df = 0.0f;
        float mindens = 0.0f;
        float maxdens = 0.0f;
        float K = 0.0f;
        std::vector<int> ordered_list_of_nodes;
    };

    // Free function to read subgraph from original data file format
    template<typename T>
    Subgraph<T> ReadSubgraph_original(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filename);
        }

        int nnodes, nlabels, nfeats;
        file.read(reinterpret_cast<char*>(&nnodes), sizeof(int));
        file.read(reinterpret_cast<char*>(&nlabels), sizeof(int));
        file.read(reinterpret_cast<char*>(&nfeats), sizeof(int));

        Subgraph<T> sg(nnodes);
        sg.setNumLabels(nlabels);
        sg.setNumFeats(nfeats);

        for (int i = 0; i < nnodes; ++i) {
            int position, truelabel;
            file.read(reinterpret_cast<char*>(&position), sizeof(int));
            file.read(reinterpret_cast<char*>(&truelabel), sizeof(int));
            
            sg.getNode(i).setPosition(position);
            sg.getNode(i).setTruelabel(truelabel);

            auto features = std::make_shared<std::vector<T>>(nfeats);
            file.read(reinterpret_cast<char*>(features->data()), nfeats * sizeof(T));
            sg.getNode(i).setFeat(features);
        }

        return sg;
    }

} // namespace opf

#endif // OPF_SUBGRAPH_HPP