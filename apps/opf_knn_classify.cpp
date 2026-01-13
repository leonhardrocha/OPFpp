#include "opf.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <stdexcept>

// Function to read precomputed distances from a file
std::vector<std::vector<float>> ReadDistances(const std::string& file_name, int& n) {
    std::ifstream file(file_name, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open file " + file_name);
    }

    file.read(reinterpret_cast<char*>(&n), sizeof(int));
    if (!file) {
        throw std::runtime_error("Could not read number of samples");
    }

    std::vector<std::vector<float>> M(n, std::vector<float>(n));
    for (int i = 0; i < n; i++) {
        file.read(reinterpret_cast<char*>(M[i].data()), n * sizeof(float));
        if (!file) {
            throw std::runtime_error("Could not read samples");
        }
    }
    return M;
}

void OPFKNNClassify(const opf::Subgraph& sg_train, opf::Subgraph& sg_test, opf::Subgraph::DistanceFunction dist_func, const std::vector<std::vector<float>>& precomputed_distances, bool use_precomputed) {
    for (auto& test_node : sg_test.nodes) {
        for (int train_node_idx : sg_train.ordered_list_of_nodes) {
            const auto& train_node = sg_train.nodes[train_node_idx];
            float weight;
            if (!use_precomputed) {
                weight = dist_func(train_node.feat, test_node.feat, sg_train.nfeats);
            } else {
                weight = precomputed_distances[train_node.position][test_node.position];
            }

            if (weight <= train_node.radius) {
                test_node.label = train_node.label;
                break;
            }
        }
    }
}

int main(int argc, char **argv){
	std::cout << "\nProgram that executes the test phase of the OPF classifier\n";
	std::cout << "\nIf you have any problem, please contact: ";
	std::cout << "\n- alexandre.falcao@gmail.com";
	std::cout << "\n- papa.joaopaulo@gmail.com\n";
	std::cout << "\nLibOPF version 3.0 (2025)\n" << std::endl;

	if((argc != 3) && (argc != 2)){
		std::cerr << "\nusage opf_knn_classify <P1> <P2>";
		std::cerr << "\nP1: test set in the OPF file format";
		std::cerr << "\nP2: precomputed distance file (leave it in blank if you are not using this resource)\n";
		return -1;
	}

	int n = 0;
	bool precomputed_distance = (argc == 3);
	std::vector<std::vector<float>> distance_value;

	std::cout << "\nReading data files ..." << std::flush;
	auto gTest = opf::Subgraph::Read(argv[1]);
	auto gTrain = opf::Subgraph::Read("classifier.opf");
	std::cout << " OK" << std::endl;

	if(precomputed_distance) {
		distance_value = ReadDistances(argv[2], n);
	}

	std::cout << "\nClassifying test set ..." << std::flush;
	auto tic = std::chrono::high_resolution_clock::now();
	OPFKNNClassify(gTrain, gTest, opf::Subgraph::EuclideanDistance, distance_value, precomputed_distance);
	auto toc = std::chrono::high_resolution_clock::now();
	std::cout << " OK" << std::endl;

	std::cout << "\nWriting output file ..." << std::flush;
	std::string out_file_name = std::string(argv[1]) + ".out";
	std::ofstream f_out(out_file_name);
	for (const auto& node : gTest.nodes) {
		f_out << node.label << "\n";
	}
	f_out.close();
	std::cout << " OK" << std::endl;
    
	std::chrono::duration<double> time_span = toc - tic;
	double time = time_span.count();
	std::cout << "\nTesting time: " << time << " seconds\n" << std::endl;

	std::string time_file_name = std::string(argv[1]) + ".time";
	std::ofstream f_time(time_file_name, std::ios_base::app);
	f_time << time << std::endl;
	f_time.close();

	return 0;
}

