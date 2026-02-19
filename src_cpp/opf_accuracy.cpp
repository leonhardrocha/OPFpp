#include "opf/Subgraph.hpp"
#include "opf/OPF.hpp"
#include <iostream>
#include <fstream>


int opf_accuracy_run(const std::string &dataset) {
    try {
        std::cout << "Reading subgraph..." << std::endl;
        auto subgraph = opf::ReadSubgraph_original<float>(dataset.c_str());
        std::cout << "Subgraph read successfully." << std::endl;
        
        std::string out_filename = dataset + ".out";
        std::ifstream outfile(out_filename);
        if (!outfile.is_open()) {
            throw std::runtime_error("Could not open .out file: " + out_filename);
        }
        
        for (int i = 0; i < subgraph.getNumNodes(); ++i) {
            int label;
            outfile >> label;
            subgraph.getNode(i).setLabel(label);
        }

        opf::OPF<float> opf_classifier;
        float acc = opf_classifier.accuracy(subgraph);

        std::cout << "Accuracy: " << acc * 100 << "%" << std::endl;
        
        std::string acc_filename = dataset + ".acc";
        std::ofstream accfile(acc_filename, std::ios_base::app);
        accfile << acc * 100 << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

#ifndef OPF_UNIT_TEST
int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: opf_accuracy <dataset>" << std::endl;
        return -1;
    }
    try {
        return opf_accuracy_run(std::string(argv[1]));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
#endif
