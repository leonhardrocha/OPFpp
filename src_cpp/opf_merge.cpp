#include "opf/Subgraph.hpp"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: opf_merge <subgraph1> <subgraph2> [subgraph3 ...]" << std::endl;
        return -1;
    }

    try {
        std::cout << "Reading initial subgraph: " << argv[1] << std::endl;
        auto merged_sg = opf::ReadSubgraph_original<float>(argv[1]);

        for (int i = 2; i < argc; ++i) {
            std::cout << "Reading and merging: " << argv[i] << std::endl;
            auto sg_to_merge = opf::ReadSubgraph_original<float>(argv[i]);
            merged_sg = opf::Subgraph<float>::merge(merged_sg, sg_to_merge);
        }

        std::cout << "Writing merged subgraph to merged.dat..." << std::endl;
        merged_sg.writeModel("merged.dat");
        std::cout << "Merged subgraph written." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
