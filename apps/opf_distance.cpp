#include "OPFpp.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

int main(int argc, char **argv){
	std::cout << "\nProgram that generates the precomputed distance file for the OPF classifier\n";
	std::cout << "\nIf you have any problem, please contact: ";
	std::cout << "\n- alexandre.falcao@gmail.com";
	std::cout << "\n- papa.joaopaulo@gmail.com\n";
	std::cout << "\nLibOPF version 3.0 (2025)\n" << std::endl;

	if(argc != 4){
		std::cerr << "\nusage opf_distance <P1> <P2> <P3>";
		std::cerr << "\nP1: Dataset in the OPF file format";
		std::cerr << "\nP2: Distance ID\n";
		std::cerr << "\n	1 - Euclidean";
		std::cerr << "\n	2 - Chi-Square";
		std::cerr << "\n	3 - Manhattan (L1)";
		std::cerr << "\n	4 - Canberra";
		std::cerr << "\n	5 - Squared Chord";
		std::cerr << "\n	6 - Squared Chi-Squared";
		std::cerr << "\n	7 - BrayCurtis";
		std::cerr << "\nP3: Distance normalization? 1- yes 0 - no" << std::endl;
		exit(-1);
	}

	auto sg = opf::Subgraph::Read(argv[1]);
	std::ofstream fp("distances.dat", std::ios::binary);
	if (!fp.is_open()) {
        std::cerr << "Error: Could not open distances.dat for writing." << std::endl;
        return -1;
    }

	int distance = std::atoi(argv[2]);
	bool normalize = (std::atoi(argv[3]) == 1);
	float max_dist = 0.0f;
    size_t num_nodes = sg.nodes.size();
	std::vector<std::vector<float>> distances(num_nodes, std::vector<float>(num_nodes));

    int num_nodes_int = static_cast<int>(num_nodes);
	fp.write(reinterpret_cast<const char*>(&num_nodes_int), sizeof(int));

	opf::Subgraph::DistanceFunction dist_func = nullptr;

	switch(distance){
		case 1:
			std::cout << "\n	Computing euclidean distance ..." << std::endl;
			dist_func = opf::Subgraph::EuclideanDistance;
			break;
		case 2:
			std::cout << "\n	Computing chi-square distance ..." << std::endl;
			dist_func = opf::Subgraph::ChiSquaredDistance;
			break;
		case 3:
			std::cout << "\n	Computing Manhattan distance ..." << std::endl;
			dist_func = opf::Subgraph::ManhattanDistance;
			break;
		case 4:
			std::cout << "\n	Computing Canberra distance ..." << std::endl;
			dist_func = opf::Subgraph::CanberraDistance;
			break;
		case 5:
			std::cout << "\n	Computing Squared Chord distance ..." << std::endl;
			dist_func = opf::Subgraph::SquaredChordDistance;
			break;
		case 6:
			std::cout << "\n	Computing Squared Chi-squared distance ..." << std::endl;
			dist_func = opf::Subgraph::SquaredChiSquaredDistance;
			break;
		case 7:
			std::cout << "\n	Computing Bray Curtis distance ..." << std::endl;
			dist_func = opf::Subgraph::BrayCurtisDistance;
			break;
		default:
			std::cerr << "\nInvalid distance ID ..." << std::endl;
			return -1;
	}

    if (dist_func) {
        for (size_t i = 0; i < num_nodes; i++){
            for (size_t j = 0; j < num_nodes; j++){
                if(i == j) {
                    distances[i][j] = 0.0;
                } else {
                    float d = dist_func(sg.nodes[i].feat, sg.nodes[j].feat, sg.nfeats);
                    distances[sg.nodes[i].position][sg.nodes[j].position] = d;
                    if(d > max_dist) max_dist = d;
                }
            }
        }
    }

	if (!normalize) max_dist = 1.0f;
	if (max_dist == 0) max_dist = 1.0f; // Avoid division by zero

	for (size_t i = 0; i < num_nodes; i++){
		for (size_t j = 0; j < num_nodes; j++){
			distances[i][j] /= max_dist;
			fp.write(reinterpret_cast<const char*>(&distances[i][j]), sizeof(float));
		}
	}

	std::cout << "\n\nDistances generated ...\n" << std::endl;
	fp.close();

	return 0;
}

