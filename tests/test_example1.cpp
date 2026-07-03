#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include "opf_test_config.h"
#include "opf_test_api.h"

int main() {
    // This test replicates the steps in examples/example1.sh
    // It assumes the executable is run from the build directory
    // where the opf_*_cpp executables are located.

    // Call the library-level functions directly (no external processes)
    if (opf_split_run(OPF_DATA_DIR "/boat.dat", 0.5f, 0.0f, 0.5f) != 0) return 1;
    if (opf_train_run("training.dat") != 0) return 1;
    if (opf_classify_run("testing.dat", "classifier.opf") != 0) return 1;
    if (opf_accuracy_run("testing.dat") != 0) return 1;

    // Cleanup generated files
    remove_file("training.dat");
    remove_file("testing.dat");
    remove_file("classifier.opf");
    remove_file("testing.dat.acc");
    remove_file("testing.dat.time");
    remove_file("testing.dat.out");
    remove_file("training.dat.model");
    remove_file("training.dat.acc");
    remove_file("training.dat.time");
    remove_file("training.dat.out");

    return 0;
}
