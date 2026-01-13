#include "opf.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

opf::Subgraph MergeSubgraphs(const opf::Subgraph& sg1, const opf::Subgraph& sg2) {
    if (sg1.nfeats != sg2.nfeats) {
        throw std::runtime_error("Invalid number of features for merging.");
    }

    opf::Subgraph merged;
    merged.nlabels = std::max(sg1.nlabels, sg2.nlabels);
    merged.nfeats = sg1.nfeats;
    
    merged.nodes.reserve(sg1.nodes.size() + sg2.nodes.size());
    merged.nodes.insert(merged.nodes.end(), sg1.nodes.begin(), sg1.nodes.end());
    merged.nodes.insert(merged.nodes.end(), sg2.nodes.begin(), sg2.nodes.end());

    return merged;
}

int main(int argc, char **argv){
	std::cout << "\nProgram that merges subgraphs\n";
	std::cout << "\nIf you have any problem, please contact: ";
	std::cout << "\n- alexandre.falcao@gmail.com";
	std::cout << "\n- papa.joaopaulo@gmail.com\n";
	std::cout << "\nLibOPF version 3.0 (2025)\n" << std::endl;

	if (argc < 3) {
		std::cerr << "\nusage opf_merge <P1> <P2> ... <Pn>";
		std::cerr << "\nP1: input dataset 1 in the OPF file format";
		std::cerr << "\nP2: input dataset 2 in the OPF file format";
		std::cerr << "\nPn: input dataset n in the OPF file format\n" << std::endl;
		return -1;
	}

	std::vector<opf::Subgraph> graphs;
	std::cout << "\nReading data sets ..." << std::flush;
	for (int i = 1; i < argc; i++) {
		graphs.push_back(opf::Subgraph::Read(argv[i]));
	}
	std::cout << " OK" << std::endl;

	if (graphs.empty()) {
        std::cerr << "No subgraphs to merge." << std::endl;
        return -1;
    }

	opf::Subgraph merged = graphs[0];
	for (size_t i = 1; i < graphs.size(); i++) {
		merged = MergeSubgraphs(merged, graphs[i]);
	}
	
	std::cout << "\nWriting data set to disk ..." << std::flush;
	merged.Write("merged.dat");
	std::cout << " OK" << std::endl;

	return 0;
}

