#include "opf/Subgraph.hpp"
#include "opf/OPF.hpp"
#include <iostream>
#include <fstream>


int opf_knn_classify_run(const std::string &test_dataset, int /*k*/) {
    try {
        std::cout << "Reading test subgraph..." << std::endl;
        auto sg_test = opf::ReadSubgraph_original<float>(test_dataset.c_str());
        std::cout << "Test subgraph read successfully." << std::endl;

        std::cout << "Reading model file..." << std::endl;
        auto sg_train = opf::Subgraph<float>::readModel((test_dataset + ".model").c_str());
        std::cout << "Model file read successfully." << std::endl;
        
        opf::OPF<float> opf_classifier;
        
        std::cout << "Classifying with kNN..." << std::endl;
        opf_classifier.knnClassifying(sg_train, sg_test);
        std::cout << "Classification completed." << std::endl;

        std::string out_filename = test_dataset + ".out";
        std::cout << "Writing output file to " << out_filename << "..." << std::endl;
        std::ofstream outfile(out_filename);
        for (int i = 0; i < sg_test.getNumNodes(); ++i) {
            outfile << sg_test.getNode(i).getLabel() << std::endl;
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
    if (argc != 3) {
        std::cerr << "Usage: opf_knn_classify <test_dataset> <model_file>" << std::endl;
        return -1;
    }
    try {
        return opf_knn_classify_run(std::string(argv[1]), 0);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
#endif
