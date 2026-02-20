#include "opf/Subgraph.hpp"
#include "opf/OPF.hpp"
#include <iostream>


int opf_learn_run(const std::string &training_dataset, const std::string &evaluation_dataset) {
    try {
        std::cout << "Reading training subgraph..." << std::endl;
        auto sg_train = opf::ReadSubgraph_original<float>(training_dataset.c_str());
        std::cout << "Training subgraph read successfully." << std::endl;
        
        std::cout << "Reading evaluation subgraph..." << std::endl;
        auto sg_eval = opf::ReadSubgraph_original<float>(evaluation_dataset.c_str());
        std::cout << "Evaluation subgraph read successfully." << std::endl;

        opf::OPF<float> opf_classifier;
        
        std::cout << "Learning..." << std::endl;
        opf_classifier.learning(sg_train, sg_eval);
        std::cout << "Learning completed." << std::endl;

        std::cout << "Final accuracy on training set: " << opf_classifier.accuracy(sg_train) * 100 << "%" << std::endl;
        std::cout << "Final accuracy on evaluation set: " << opf_classifier.accuracy(sg_eval) * 100 << "%" << std::endl;

        std::cout << "Writing model file..." << std::endl;
        sg_train.writeModel("classifier.opf");
        std::cout << "Model file written." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

#ifndef OPF_UNIT_TEST
int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: opf_learn <training_dataset> <evaluation_dataset>" << std::endl;
        return -1;
    }
    try {
        return opf_learn_run(std::string(argv[1]), std::string(argv[2]));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
#endif
