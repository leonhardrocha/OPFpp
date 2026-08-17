#include "opf/Subgraph.hpp"
#include "opf/Utils.hpp"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: opf_fold <dataset> <k>" << std::endl;
        return -1;
    }

    try {
        int k = std::stoi(argv[2]);
        if (k <= 0) {
            throw std::runtime_error("k must be a positive integer.");
        }

        std::cout << "Reading original dataset..." << std::endl;
        auto original_sg = opf::ReadSubgraph_original<float>(argv[1]);
        std::cout << "Dataset read successfully." << std::endl;
        
        std::cout << "Creating " << k << " folds..." << std::endl;
        auto folds = opf::kFold(original_sg, k);
        std::cout << "Folds created." << std::endl;

        for (int i = 0; i < k; ++i) {
            std::string filename = "fold_" + std::to_string(i + 1) + ".dat";
            std::cout << "Writing " << filename << "..." << std::endl;
            folds[i].writeModel(filename);
        }

        std::cout << "All files written." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
