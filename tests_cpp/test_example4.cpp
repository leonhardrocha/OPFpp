#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include "opf_test_config.h"
#include "opf_test_api.h"

int main() {
    // This test replicates the steps in examples/example4.sh
#ifdef _WIN32
    const char* opf_distance = "opf_distance_cpp.exe";
    const char* opf_split = "opf_split_cpp.exe";
    const char* opf_learn = "opf_learn_cpp.exe";
    const char* opf_classify = "opf_classify_cpp.exe";
    const char* opf_accuracy = "opf_accuracy_cpp.exe";
#else
    const char* opf_distance = "./opf_distance_cpp";
    const char* opf_split = "./opf_split_cpp";
    const char* opf_learn = "./opf_learn_cpp";
    const char* opf_classify = "./opf_classify_cpp";
    const char* opf_accuracy = "./opf_accuracy_cpp";
#endif
    const char* data_file = "./data/cone-torus.dat";

    (void)0; // nothing platform-specific needed; calling API wrappers directly

    if (opf_distance_run("./data/cone-torus.dat", 3) != 0) return 1;
    if (opf_split_run("./data/cone-torus.dat", 0.3f, 0.2f, 0.5f) != 0) return 1;
    if (opf_learn_run("training.dat", "evaluating.dat") != 0) return 1;
    if (opf_classify_run("testing.dat", "classifier.opf") != 0) return 1;
    if (opf_accuracy_run("testing.dat") != 0) return 1;

    return 0;
}
