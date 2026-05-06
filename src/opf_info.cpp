#include "opf/Subgraph.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: opf_info <dataset>" << std::endl;
        return -1;
    }

    try {
        std::ifstream file(argv[1], std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + std::string(argv[1]));
        }

        int nnodes, nlabels, nfeats;
        file.read(reinterpret_cast<char*>(&nnodes), sizeof(int));
        file.read(reinterpret_cast<char*>(&nlabels), sizeof(int));
        file.read(reinterpret_cast<char*>(&nfeats), sizeof(int));
        
        std::cout << "Information for " << argv[1] << std::endl;
        std::cout << "-------------------------" << std::endl;
        std::cout << "Number of nodes: " << nnodes << std::endl;
        std::cout << "Number of labels: " << nlabels << std::endl;
        std::cout << "Number of features: " << nfeats << std::endl;
        std::cout << "-------------------------" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
