#include "opf/Subgraph.hpp"
#include "opf/Utils.hpp"
#include <iostream>
#include <string>
#include <vector>


// Testable API wrapper
int opf_split_run(const std::string &dataset, float train_p, float eval_p, float test_p) {
    try {
        std::cout << "Reading original dataset..." << std::endl;
        auto original_sg = opf::ReadSubgraph_original<float>(dataset.c_str());
        std::cout << "Dataset read successfully." << std::endl;

        opf::Subgraph<float> train_sg, eval_sg, test_sg, temp_sg;

        std::cout << "Splitting dataset..." << std::endl;
        opf::split(original_sg, temp_sg, test_sg, train_p + eval_p);

        if (eval_p > 0) {
            opf::split(temp_sg, train_sg, eval_sg, train_p / (train_p + eval_p));
        } else {
            train_sg = temp_sg;
        }
        std::cout << "Split completed." << std::endl;

        std::cout << "Writing training.dat..." << std::endl;
        train_sg.writeModel("training.dat");
        
        if (eval_p > 0) {
            std::cout << "Writing evaluating.dat..." << std::endl;
            eval_sg.writeModel("evaluating.dat");
        }

        std::cout << "Writing testing.dat..." << std::endl;
        test_sg.writeModel("testing.dat");

        std::cout << "All files written." << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

#ifndef OPF_UNIT_TEST
int main(int argc, char **argv) {
    if (argc != 5) {
        std::cerr << "Usage: opf_split <dataset> <train_perc> <eval_perc> <test_perc>" << std::endl;
        return -1;
    }

    try {
        float train_p = std::stof(argv[2]);
        float eval_p = std::stof(argv[3]);
        float test_p = std::stof(argv[4]);

        if (std::abs(train_p + eval_p + test_p - 1.0) > 1e-6) {
            throw std::runtime_error("Percentages must sum to 1.0");
        }

        return opf_split_run(std::string(argv[1]), train_p, eval_p, test_p);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
#endif

