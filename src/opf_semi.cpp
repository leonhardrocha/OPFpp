#include "opf/Subgraph.hpp"
#include "opf/OPF.hpp"
#include <iostream>
#include <memory>


int opf_semi_run(const std::string &labeled, const std::string &unlabeled, const std::string &eval, const std::string &model_filename) {
    try {
        std::cout << "Reading labeled subgraph..." << std::endl;
        auto sg_labeled = opf::ReadSubgraph_original<float>(labeled.c_str());
        std::cout << "Labeled subgraph read successfully." << std::endl;
        
        std::cout << "Reading unlabeled subgraph..." << std::endl;
        auto sg_unlabeled = opf::ReadSubgraph_original<float>(unlabeled.c_str());
        std::cout << "Unlabeled subgraph read successfully." << std::endl;

        std::unique_ptr<opf::Subgraph<float>> sg_eval = nullptr;
        if (!eval.empty()) {
            std::cout << "Reading evaluation subgraph..." << std::endl;
            sg_eval = std::make_unique<opf::Subgraph<float>>(opf::ReadSubgraph_original<float>(eval.c_str()));
            std::cout << "Evaluation subgraph read successfully." << std::endl;
        }

        opf::OPF<float> opf_classifier;
        
        std::cout << "Running semi-supervised learning..." << std::endl;
        auto final_sg = opf_classifier.semiSupervisedLearning(sg_labeled, sg_unlabeled, sg_eval.get());
        std::cout << "Semi-supervised learning completed." << std::endl;
        
        std::cout << "Writing model file..." << std::endl;
        final_sg.writeModel(model_filename.c_str());
        std::cout << "Model file written to " << model_filename << std::endl;
        
        std::cout << "Model file written." << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

#ifndef OPF_UNIT_TEST
int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: opf_semi <labeled_dataset> <unlabeled_dataset> [evaluation_dataset] [model_filename]" << std::endl;
        return -1;
    }
    try {
        if (argc == 5) {
            return opf_semi_run(std::string(argv[1]), std::string(argv[2]), std::string(argv[3]), std::string(argv[4]));
        } else {
            return opf_semi_run(std::string(argv[1]), std::string(argv[2]), "", "classifier.opf");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
#endif
