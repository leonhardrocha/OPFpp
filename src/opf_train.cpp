#include "opf/Subgraph.hpp"
#include "opf/OPF.hpp"
#include <iostream>


int opf_train_run(const std::string &dataset) {
    try {
        std::cout << "Reading subgraph..." << std::endl;
        auto subgraph = opf::ReadSubgraph_original<float>(dataset.c_str());
        std::cout << "Subgraph read successfully." << std::endl;

        opf::OPF<float> opf_classifier;
        
        std::cout << "Training..." << std::endl;
        opf_classifier.training(subgraph);
        std::cout << "Training completed." << std::endl;

        std::cout << "Writing model file..." << std::endl;
        subgraph.writeModel("classifier.opf");
        std::cout << "Model file written." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}


#ifndef OPF_UNIT_TEST
int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: opf_train <dataset>" << std::endl;
        return -1;
    }

    try {
        return opf_train_run(std::string(argv[1]));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
#endif