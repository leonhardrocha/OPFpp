#include "opf/Subgraph.hpp"
#include "opf/Distance.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

void printUsage() {
    std::cerr << "Usage: opf_distance <dataset> <distance_id>" << std::endl;
    std::cerr << "Distance IDs:" << std::endl;
    std::cerr << "1 - Euclidean" << std::endl;
    std::cerr << "2 - Chi-Square" << std::endl;
    std::cerr << "3 - Manhattan" << std::endl;
    std::cerr << "4 - Canberra" << std::endl;
    std::cerr << "5 - Squared Chord" << std::endl;
    std::cerr << "6 - Squared Chi-Squared" << std::endl;
    std::cerr << "7 - Bray-Curtis" << std::endl;
}

int opf_distance_run(const std::string &dataset, int distance_id) {
    try {
        std::cout << "Reading subgraph..." << std::endl;
        auto sg = opf::ReadSubgraph_original<float>(dataset.c_str());
        std::cout << "Subgraph read successfully." << std::endl;

        std::vector<std::vector<float>> dist_matrix(sg.getNumNodes(), std::vector<float>(sg.getNumNodes()));

        std::cout << "Computing distances..." << std::endl;
        for (int i = 0; i < sg.getNumNodes(); ++i) {
            for (int j = 0; j < sg.getNumNodes(); ++j) {
                if (i == j) {
                    dist_matrix[i][j] = 0.0f;
                    continue;
                }

                const auto& f1 = *sg.getNode(i).getFeat();
                const auto& f2 = *sg.getNode(j).getFeat();

                switch (distance_id) {
                    case 1:
                        dist_matrix[i][j] = opf::distance::euclDist(f1, f2);
                        break;
                    case 2:
                        dist_matrix[i][j] = opf::distance::chiSquaredDist(f1, f2);
                        break;
                    case 3:
                        dist_matrix[i][j] = opf::distance::manhattanDist(f1, f2);
                        break;
                    case 4:
                        dist_matrix[i][j] = opf::distance::canberraDist(f1, f2);
                        break;
                    case 5:
                        dist_matrix[i][j] = opf::distance::squaredChordDist(f1, f2);
                        break;
                    case 6:
                        dist_matrix[i][j] = opf::distance::squaredChiSquaredDist(f1, f2);
                        break;
                    case 7:
                        dist_matrix[i][j] = opf::distance::brayCurtisDist(f1, f2);
                        break;
                    default:
                        std::cerr << "Invalid distance ID." << std::endl;
                        printUsage();
                        return -1;
                }
            }
        }
        std::cout << "Distances computed." << std::endl;

        std::cout << "Writing distances to distances.dat..." << std::endl;
        std::ofstream outfile("distances.dat", std::ios::binary);
        int nnodes = sg.getNumNodes();
        outfile.write(reinterpret_cast<const char*>(&nnodes), sizeof(int));
        for (int i = 0; i < nnodes; ++i) {
            outfile.write(reinterpret_cast<const char*>(dist_matrix[i].data()), nnodes * sizeof(float));
        }
        outfile.close();
        std::cout << "Distances written to distances.dat." << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

#ifndef OPF_UNIT_TEST
int main(int argc, char **argv) {
    if (argc != 3) {
        printUsage();
        return -1;
    }

    try {
        int distance_id = std::stoi(argv[2]);
        return opf_distance_run(std::string(argv[1]), distance_id);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
#endif
