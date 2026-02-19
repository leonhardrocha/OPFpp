#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include "opf_test_config.h"
#include "opf_test_api.h"

int main() {
    // This test replicates the steps in examples/example5.sh
#ifdef _WIN32
    const char* opf_split = "opf_split_cpp.exe";
    const char* opf_cluster = "opf_cluster_cpp.exe";
    const char* opf_knn_classify = "opf_knn_classify_cpp.exe";
    const char* opf_accuracy = "opf_accuracy_cpp.exe";
#else
    const char* opf_split = "./opf_split_cpp";
    const char* opf_cluster = "./opf_cluster_cpp";
    const char* opf_knn_classify = "./opf_knn_classify_cpp";
    const char* opf_accuracy = "./opf_accuracy_cpp";
#endif
    
    char command[512];

    const char* data_files[] = {"../data/data1.dat", "../data/data2.dat", "../data/data3.dat", "../data/data4.dat", "../data/data5.dat"};
    const int ks[] = {100, 100, 20, 100, 50};
    const int p2s[] = {1, 2, 2, 2, 1};
    const double p3s[] = {0.2, 0.01, 0.001, 0.02, 0.2};

    for (int i = 0; i < 5; ++i) {
        if (opf_split_run(std::string(data_files[i]), 0.8f, 0.0f, 0.2f) != 0) return 1;
        char argsbuf[128];
        sprintf(argsbuf, "training.dat %d %d %f", ks[i], p2s[i], p3s[i]);
        if (opf_cluster_run("training.dat") != 0) return 1;
        if (opf_knn_classify_run("testing.dat", ks[i]) != 0) return 1;
        if (opf_accuracy_run("testing.dat") != 0) return 1;
    }

    return 0;
}
