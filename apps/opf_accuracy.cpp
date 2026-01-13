#include "opf.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>

float CalculateAccuracy(const opf::Subgraph& sg) {
    if (sg.nodes.empty()) {
        return 0.0f;
    }

    int max_label = 0;
    for (const auto& node : sg.nodes) {
        if (node.truelabel > max_label) {
            max_label = node.truelabel;
        }
        if (node.label > max_label) {
            max_label = node.label;
        }
    }

    std::vector<std::vector<float>> error_matrix(max_label + 1, std::vector<float>(2, 0.0f));
    std::vector<int> nclass(max_label + 1, 0);
    int nlabels = 0;

    for (const auto& node : sg.nodes) {
        if (node.truelabel > 0 && static_cast<size_t>(node.truelabel) < nclass.size()) {
            nclass[node.truelabel]++;
        }
    }

    for (const auto& node : sg.nodes) {
        if (node.truelabel != node.label) {
            if (node.truelabel > 0 && static_cast<size_t>(node.truelabel) < error_matrix.size()) {
                error_matrix[node.truelabel][1]++;
            }
            if (node.label > 0 && static_cast<size_t>(node.label) < error_matrix.size()) {
                error_matrix[node.label][0]++;
            }
        }
    }

    for (int i = 1; i <= max_label; i++) {
        if (nclass[i] != 0) {
            error_matrix[i][1] /= static_cast<float>(nclass[i]);
            if (sg.nodes.size() > nclass[i]) {
                error_matrix[i][0] /= static_cast<float>(sg.nodes.size() - nclass[i]);
            }
            nlabels++;
        }
    }

    float total_error = 0.0f;
    for (int i = 1; i <= max_label; i++) {
        if (nclass[i] != 0) {
            total_error += (error_matrix[i][0] + error_matrix[i][1]);
        }
    }

    if (nlabels == 0) {
        return 1.0f; // Or handle as an error case
    }

    return 1.0f - (total_error / (2.0f * nlabels));
}

int main(int argc, char **argv){
	std::cout << "\nProgram that computes OPF accuracy of a given set\n";
	std::cout << "\nIf you have any problem, please contact: ";
	std::cout << "\n- alexandre.falcao@gmail.com";
	std::cout << "\n- papa.joaopaulo@gmail.com\n";
	std::cout << "\nLibOPF version 3.0 (2025)\n" << std::endl;

	if(argc != 2){
		std::cerr << "\nusage opf_accuracy <P1>";
		std::cerr << "\nP1: data set in the OPF file format" << std::endl;
		return -1;
	}

	std::cout << "\nReading data file ..." << std::flush;
	auto g = opf::Subgraph::Read(argv[1]);
	std::cout << " OK" << std::endl;

	std::cout << "Reading output file ..." << std::flush;
	std::string output_file_name = std::string(argv[1]) + ".out";
	std::ifstream f_out(output_file_name);
	if(!f_out){
		std::cerr << "\nunable to open file " << output_file_name << std::endl;
		return -1;
	}
	for (size_t i = 0; i < g.nodes.size(); i++) {
		if (!(f_out >> g.nodes[i].label)) {
			std::cerr << "\nError reading node label" << std::endl;
			return -1;
		}
	}
	f_out.close();
	std::cout << " OK" << std::endl;

	std::cout << "Computing accuracy ..." << std::flush;
	float Acc = CalculateAccuracy(g);
	std::cout << "\nAccuracy: " << Acc*100 << "%" << std::endl;

	std::cout << "Writing accuracy in output file ..." << std::flush;
	std::string acc_file_name = std::string(argv[1]) + ".acc";
	std::ofstream f_acc(acc_file_name, std::ios_base::app);
	f_acc << Acc*100 << std::endl;
	f_acc.close();
	std::cout << " OK" << std::endl;

	return 0;
}

