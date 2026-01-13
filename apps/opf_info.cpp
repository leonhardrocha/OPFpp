#include "opf.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char **argv){
	std::cout << "\nProgram that gives information about the OPF file\n";
	std::cout << "\nIf you have any problem, please contact: ";
	std::cout << "\n- alexandre.falcao@gmail.com";
	std::cout << "\n- papa.joaopaulo@gmail.com\n";
	std::cout << "\nLibOPF version 3.0 (2025)\n" << std::endl;

	if(argc != 2) {
		std::cerr << "\nusage opf_info <P1>";
		std::cerr << "\nP1: OPF file\n" << std::endl;
		return -1;
	}

	try {
		auto g = opf::Subgraph::Read(argv[1]);
		
		std::cout << "\nInformations about " << argv[1] << " file\n --------------------------------" << std::endl;
		std::cout << "Data size: " << g.nodes.size() << std::endl;
		std::cout << "Features size: " << g.nfeats << std::endl;
		std::cout << "Labels number: " << g.nlabels << std::endl;
		std::cout << "--------------------------------\n" << std::endl;

	} catch (const std::runtime_error& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}

