#pragma once

#include <vector>
#include <string>
#include "IComponent.h"

namespace ecs {

struct CFeatures : IComponent {
    std::vector<float> values;
    
    CFeatures() = default;
    explicit CFeatures(const std::vector<float> &v) : values(v) {}
    explicit CFeatures(std::vector<float> &&v) : values(std::move(v)) {}
};

struct CLabel : IComponent {
    int label{-1};
    
    CLabel() = default;
    explicit CLabel(int l) : label(l) {}
};

struct CIOPath : IComponent {
    std::string path;
    
    CIOPath() = default;
    explicit CIOPath(const std::string &p) : path(p) {}
    explicit CIOPath(std::string &&p) : path(std::move(p)) {}
};

struct CFlags : IComponent {
    bool isTraining{false};
    bool isEvaluation{false};
    bool isTesting{false};
    
    CFlags() = default;
    CFlags(bool training, bool evaluation, bool testing)
        : isTraining(training), isEvaluation(evaluation), isTesting(testing) {}
};

struct CSamples : IComponent {
    std::vector<int> indices;  // Sample indices in dataset
    
    CSamples() = default;
    explicit CSamples(const std::vector<int> &idx) : indices(idx) {}
    explicit CSamples(std::vector<int> &&idx) : indices(std::move(idx)) {}
};

struct CSubgraphRange : IComponent {
    int start{0};
    int end{0};
    
    CSubgraphRange() = default;
    explicit CSubgraphRange(int s, int e) : start(s), end(e) {}
};

struct CSplitParams : IComponent {
    float train_percentage{0.5f};
    float eval_percentage{0.0f};
    float test_percentage{0.5f};
    
    CSplitParams() = default;
    CSplitParams(float train, float eval, float test)
        : train_percentage(train), eval_percentage(eval), test_percentage(test) {}
};

struct CSerializationMeta : IComponent {
    std::string format{"json"};  // "json", "binary", etc.
    std::string version{"1.0"};
    
    CSerializationMeta() = default;
    explicit CSerializationMeta(const std::string &fmt, const std::string &ver = "1.0")
        : format(fmt), version(ver) {}
};

// ============================================================================
// Training & Classification Components
// ============================================================================

struct CModelParams : IComponent {
    std::vector<int> prototypes;              // Indices of prototype nodes
    std::vector<float> pathvalues;            // Path values for each node
    std::vector<int> predecessors;            // Predecessor node for each node in IFT
    std::vector<int> ordered_nodes;           // Nodes ordered by path value (for classification)
    std::vector<int> node_labels;             // Assigned labels for each node
    int num_features{0};                      // Feature dimension
    int num_nodes{0};                         // Total number of nodes
    
    CModelParams() = default;
    explicit CModelParams(int nfeats, int nnodes)
        : num_features(nfeats), num_nodes(nnodes),
          pathvalues(nnodes, -1.0f),
          predecessors(nnodes, -1),
          node_labels(nnodes, -1) {}
};

struct CEvalMetrics : IComponent {
    float accuracy{-1.0f};                    // Classification accuracy (0.0 to 1.0)
    float training_time_ms{0.0f};             // Training duration in milliseconds
    int num_prototypes{0};                    // Number of prototypes selected
    int num_samples_trained{0};               // Total samples in training set
    int correct_classifications{0};           // Count of correct predictions
    int total_classifications{0};             // Total predictions made
    std::string status{""};                   // "trained", "evaluated", "error", etc.
    
    CEvalMetrics() = default;
};

} // namespace ecs
