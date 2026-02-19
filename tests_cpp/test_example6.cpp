#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>
#include "opf_test_config.h"
#include "opf_test_api.h"

// Helper function to check if a file exists
bool file_exists(const char* filename) {
    std::ifstream infile(filename);
    return infile.good();
}

// Helper function to copy a file
void copy_file(const char* src, const char* dest) {
    std::ifstream source(src, std::ios::binary);
    std::ofstream destination(dest, std::ios::binary);
    destination << source.rdbuf();
}

int main() {
    // This test replicates the steps in examples/example6.sh
#ifdef _WIN32
    const char* opf_split = "opf_split_cpp.exe";
    const char* opf_semi = "opf_semi_cpp.exe";
    const char* opf_classify = "opf_classify_cpp.exe";
    const char* opf_accuracy = "opf_accuracy_cpp.exe";
#else
    const char* opf_split = "./opf_split_cpp";
    const char* opf_semi = "./opf_semi_cpp";
    const char* opf_classify = "./opf_classify_cpp";
    const char* opf_accuracy = "./opf_accuracy_cpp";
#endif
    const char* data_file = "../data/saturn.dat";

    if (opf_split_run("../data/saturn.dat", 0.6f, 0.2f, 0.2f) != 0) return 1;
    copy_file("training.dat", "Z1.dat");
    copy_file("testing.dat", "Z3.dat");
    remove("training.dat");
    remove("testing.dat");
    if (opf_split_run("Z1.dat", 0.4f, 0.0f, 0.6f) != 0) return 1;
    copy_file("training.dat", "Z1LINE.dat");
    copy_file("testing.dat", "Z1DOUBLELINE.dat");
    remove("training.dat");
    remove("testing.dat");

    // First part
    if (opf_semi_run("Z1LINE.dat", "Z1DOUBLELINE.dat", "") != 0) return 1;
    if (opf_classify_run("Z3.dat", "classifier.opf") != 0) return 1;
    if (opf_accuracy_run("Z3.dat") != 0) return 1;

    // remove intermediate files
    remove("classifier.opf");
    remove("Z1.dat.acc");
    remove("Z1.dat.time");
    remove("Z1.dat.out");
    remove("Z3.dat.acc");
    remove("Z3.dat.time");
    remove("Z3.dat.out");

    // Second part
    if (opf_semi_run("Z1LINE.dat", "Z1DOUBLELINE.dat", "evaluating.dat") != 0) return 1;
    if (opf_classify_run("Z3.dat", "classifier.opf") != 0) return 1;
    if (opf_accuracy_run("Z3.dat") != 0) return 1;

    // Final cleanup
    remove("Z1.dat");
    remove("Z3.dat");
    remove("Z1LINE.dat");
    remove("Z1DOUBLELINE.dat");
    remove("evaluating.dat");
    remove("classifier.opf");
    remove("Z3.dat.acc");
    remove("Z3.dat.time");
    remove("Z3.dat.out");

    return 0;
}
