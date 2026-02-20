#include "opf/Subgraph.hpp"
#include "opf/OPF.hpp"
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "Usage: opf_pruning <training_dataset> <evaluation_dataset> <desired_accuracy>" << std::endl;
        return -1;
    }

    try {
        std::cout << "Reading training subgraph..." << std::endl;
        auto sg_train = opf::ReadSubgraph_original<float>(argv[1]);
        std::cout << "Training subgraph read successfully." << std::endl;
        
        std::cout << "Reading evaluation subgraph..." << std::endl;
        auto sg_eval = opf::ReadSubgraph_original<float>(argv[2]);
        std::cout << "Evaluation subgraph read successfully." << std::endl;

        float desired_acc = std::stof(argv[3]);

        opf::OPF<float> opf_classifier;
        
        std::cout << "Pruning..." << std::endl;
        float pruning_rate = opf_classifier.pruning(sg_train, sg_eval, desired_acc);
        std::cout << "Pruning completed." << std::endl;

        std::cout << "Final pruning rate: " << pruning_rate * 100 << "%" << std::endl;
        
        std::cout << "Writing model file..." << std::endl;
        sg_train.writeModel("classifier.opf");
        std::cout << "Model file written." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
