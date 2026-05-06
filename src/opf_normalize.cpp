#include "opf/Subgraph.hpp"
#include "opf/OPF.hpp"
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: opf_normalize <input_dataset> <output_dataset>" << std::endl;
        return -1;
    }

    try {
        std::cout << "Reading subgraph..." << std::endl;
        auto subgraph = opf::ReadSubgraph_original<float>(argv[1]);
        std::cout << "Subgraph read successfully." << std::endl;

        opf::OPF<float> opf_classifier;
        
        std::cout << "Normalizing..." << std::endl;
        opf_classifier.normalize(subgraph);
        std::cout << "Normalization completed." << std::endl;

        std::cout << "Writing normalized subgraph..." << std::endl;
        // The writeModel function will save all subgraph's data, including the normalized features.
        // A function to write in the original format would be needed to make it compatible with the C tools.
        subgraph.writeModel(argv[2]);
        std::cout << "Normalized subgraph written to " << argv[2] << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
