#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>
#include "opf_test_config.h"
#include "opf_test_api.h"

// Helper functions moved to tests_cpp/opf_test_api.h

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
    const char* data_file = OPF_DATA_DIR "/saturn.dat";

    if (opf_split_run(OPF_DATA_DIR "/saturn.dat", 0.6f, 0.2f, 0.2f) != 0) return 1;
    if (!copy_file("training.dat", "Z1.dat")) return 1;
    if (!copy_file("testing.dat", "Z3.dat")) return 1;
    remove_file("training.dat");
    remove_file("testing.dat");
    if (opf_split_run("Z1.dat", 0.4f, 0.0f, 0.6f) != 0) return 1;
    if (!copy_file("training.dat", "Z1LINE.dat")) return 1;
    if (!copy_file("testing.dat", "Z1DOUBLELINE.dat")) return 1;
    remove_file("training.dat");
    remove_file("testing.dat");

    // First part
    if (opf_semi_run("Z1LINE.dat", "Z1DOUBLELINE.dat", "", "classifier.opf") != 0) return 1;
    if (opf_classify_run("Z3.dat", "classifier.opf") != 0) return 1;
    if (opf_accuracy_run("Z3.dat") != 0) return 1;

    // remove intermediate files
    remove_file("classifier.opf");
    remove_file("Z1.dat.acc");
    remove_file("Z1.dat.time");
    remove_file("Z1.dat.out");
    remove_file("Z3.dat.acc");
    remove_file("Z3.dat.time");
    remove_file("Z3.dat.out");

    // Second part
    if (opf_semi_run("Z1.dat", "Z1DOUBLELINE.dat", "evaluating.dat", "classifier.opf") != 0) return 1;
    if (opf_classify_run("Z3.dat", "classifier.opf") != 0) return 1;
    if (opf_accuracy_run("Z3.dat") != 0) return 1;

    // Final cleanup
    remove_file("Z1.dat");
    remove_file("Z3.dat");
    remove_file("Z1LINE.dat");
    remove_file("Z1DOUBLELINE.dat");
    remove_file("evaluating.dat");
    remove_file("classifier.opf");
    remove_file("Z3.dat.acc");
    remove_file("Z3.dat.time");
    remove_file("Z3.dat.out");

    return 0;
}
