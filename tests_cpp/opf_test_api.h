#pragma once
#include <string>

#include <fstream>
#include <cstdio>
#include <iostream>

// Small API so tests can call executable logic directly instead of spawning
// external processes. Implemented in each corresponding src_cpp/opf_*.cpp.
int opf_split_run(const std::string &dataset, float train_p, float eval_p, float test_p);
int opf_train_run(const std::string &dataset);
int opf_classify_run(const std::string &test_dataset, const std::string &model_file);
int opf_accuracy_run(const std::string &dataset);
int opf_learn_run(const std::string &training_dataset, const std::string &evaluation_dataset);
int opf_distance_run(const std::string &dataset, int distance_id);
int opf_cluster_run(const std::string &dataset);
int opf_knn_classify_run(const std::string &dataset, const std::string &model_file);
int opf_semi_run(const std::string &labeled, const std::string &unlabeled, const std::string &eval, const std::string &model_filename);

// Test helper utilities: file existence, copy, and remove operations.
inline bool file_exists(const std::string &filename) {
	std::ifstream infile(filename, std::ios::binary);
	return infile.good();
}

inline bool copy_file(const std::string &src, const std::string &dest) {
	std::ifstream source(src, std::ios::binary);
	if (!source.is_open()) {
		std::cerr << "Error: Could not open source file: " << src << std::endl;
		return false;
	}

	std::ofstream destination(dest, std::ios::binary);
	if (!destination.is_open()) {
		std::cerr << "Error: Could not open destination file: " << dest << std::endl;
		source.close();
		return false;
	}

	destination << source.rdbuf();

	if (!destination.good()) {
		std::cerr << "Error: Failed to write to destination file: " << dest << std::endl;
		source.close();
		destination.close();
		return false;
	}

	source.close();
	destination.close();
	return true;
}

inline bool remove_file(const std::string &path) {
	int rc = std::remove(path.c_str());
	if (rc != 0 && file_exists(path)) {
		std::cerr << "Error: Failed to remove file: " << path << std::endl;
		return false;
	}
	return true;
}

// Overloads for convenience with C-style strings
inline bool copy_file(const char *src, const char *dest) { return copy_file(std::string(src), std::string(dest)); }
inline bool remove_file(const char *path) { return remove_file(std::string(path)); }
inline bool file_exists(const char *path) { return file_exists(std::string(path)); }
