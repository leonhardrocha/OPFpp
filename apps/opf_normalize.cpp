#include "opf.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <numeric>

void NormalizeFeatures(opf::Subgraph& sg) {
    if (sg.nodes.empty() || sg.nfeats == 0) {
        return;
    }

    std::vector<float> mean(sg.nfeats, 0.0f);
    std::vector<float> std_dev(sg.nfeats, 0.0f);

    for (int i = 0; i < sg.nfeats; ++i) {
        for (const auto& node : sg.nodes) {
            mean[i] += node.feat[i];
        }
        mean[i] /= sg.nodes.size();
    }

    for (int i = 0; i < sg.nfeats; ++i) {
        for (const auto& node : sg.nodes) {
            std_dev[i] += std::pow(node.feat[i] - mean[i], 2);
        }
        std_dev[i] = std::sqrt(std_dev[i] / sg.nodes.size());
        if (std_dev[i] == 0.0f) {
            std_dev[i] = 1.0f;
        }
    }

    for (auto& node : sg.nodes) {
        for (int i = 0; i < sg.nfeats; ++i) {
            node.feat[i] = (node.feat[i] - mean[i]) / std_dev[i];
        }
    }
}

int main(int argc, char **argv){
	std::cout << "\nProgram that normalizes data for the OPF classifier\n";
	std::cout << "\nIf you have any problem, please contact: ";
	std::cout << "\n- alexandre.falcao@gmail.com";
	std::cout << "\n- papa.joaopaulo@gmail.com\n";
	std::cout << "\nLibOPF version 3.0 (2025)\n" << std::endl;

	if(argc != 3){
		std::cerr << "\nusage opf_normalize <P1> <P2>";
		std::cerr << "\nP1: input dataset in the OPF file format";
		std::cerr << "\nP2: normalized output dataset in the OPF file format\n" << std::endl;
		return -1;
	}

	std::cout << "\nReading data set ..." << std::flush;
	auto g = opf::Subgraph::Read(argv[1]);
	std::cout << " OK" << std::endl;

	std::cout << "\nNormalizing data set ..." << std::flush;
    NormalizeFeatures(g);
	std::cout << " OK" << std::endl;
    
	std::cout << "\nWriting normalized data set to disk ..." << std::flush;
    g.Write(argv[2]);
	std::cout << " OK" << std::endl;

	return 0;
}

