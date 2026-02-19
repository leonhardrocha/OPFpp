#include "opf/Subgraph.hpp"
#include "opf/OPF.hpp"
#include <iostream>
#include <fstream>
#include <string>


int opf_cluster_run(const std::string &dataset) {
    try {
        std::cout << "Reading subgraph..." << std::endl;
        auto subgraph = opf::ReadSubgraph_original<float>(dataset.c_str());
        std::cout << "Subgraph read successfully." << std::endl;

        // --- Placeholder for opf_BestkMinCut logic ---
        std::cout << "Running placeholder for BestKMinCut..." << std::endl;
        for (int i = 0; i < subgraph.getNumNodes(); ++i) {
            subgraph.getNode(i).setDens(static_cast<float>(rand()) / RAND_MAX);
            for (int j = 0; j < subgraph.getNumNodes(); ++j) {
                if (i != j) {
                    subgraph.getNode(i).addToAdj(j);
                }
            }
        }

        opf::OPF<float> opf_classifier;

        std::cout << "Clustering..." << std::endl;
        opf_classifier.clustering(subgraph);
        std::cout << "Clustering completed with " << subgraph.getNumLabels() << " clusters." << std::endl;

        // Label propagation logic
        if (subgraph.getNode(0).getTruelabel() != 0) {
            int max_label = 0;
            for (int i = 0; i < subgraph.getNumNodes(); ++i) {
                if (subgraph.getNode(i).getRoot() == i) {
                    subgraph.getNode(i).setLabel(subgraph.getNode(i).getTruelabel());
                } else {
                    subgraph.getNode(i).setLabel(subgraph.getNode(subgraph.getNode(i).getRoot()).getLabel());
                }
                if (subgraph.getNode(i).getLabel() > max_label) {
                    max_label = subgraph.getNode(i).getLabel();
                }
            }
            subgraph.setNumLabels(max_label);
        } else {
            for (int i = 0; i < subgraph.getNumNodes(); i++) {
                subgraph.getNode(i).setTruelabel(subgraph.getNode(i).getLabel() + 1);
            }
        }
        std::string model_filename = dataset + ".model";
        std::cout << "Writing model file to " << model_filename << "..." << std::endl;
        subgraph.writeModel(model_filename.c_str());
        std::cout << "Model file written." << std::endl;

        std::string out_filename = dataset + ".out";
        std::cout << "Writing output file to " << out_filename << "..." << std::endl;
        std::ofstream outfile(out_filename);
        for (int i = 0; i < subgraph.getNumNodes(); ++i) {
            outfile << subgraph.getNode(i).getLabel() << std::endl;
        }
        outfile.close();
        std::cout << "Output file written." << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

#ifndef OPF_UNIT_TEST
int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: opf_cluster <unlabeled_dataset> [options...]" << std::endl;
        return -1;
    }


    try {
        return opf_cluster_run(std::string(argv[1]));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
#endif