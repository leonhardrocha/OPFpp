#pragma once
#include <string>

// Small API so tests can call executable logic directly instead of spawning
// external processes. Implemented in each corresponding src_cpp/opf_*.cpp.
int opf_split_run(const std::string &dataset, float train_p, float eval_p, float test_p);
int opf_train_run(const std::string &dataset);
int opf_classify_run(const std::string &test_dataset, const std::string &model_file);
int opf_accuracy_run(const std::string &dataset);
int opf_learn_run(const std::string &training_dataset, const std::string &evaluation_dataset);
int opf_distance_run(const std::string &dataset, int distance_id);
int opf_cluster_run(const std::string &dataset);
int opf_knn_classify_run(const std::string &dataset, int k);
int opf_semi_run(const std::string &labeled_file, const std::string &unlabeled_file, const std::string &output_file);
