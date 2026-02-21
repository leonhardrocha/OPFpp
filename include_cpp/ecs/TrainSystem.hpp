#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <numeric>
#include <limits>
#include <chrono>
#include "System.h"
#include "EntityRegistry.h"
#include "Components.hpp"

namespace ecs {

// Simple distance function (Euclidean) used in training
inline float euclidean_distance(const float* f1, const float* f2, int n) {
    if (f1 == nullptr || f2 == nullptr) return std::numeric_limits<float>::max();
    float dist = 0.0f;
    for (int i = 0; i < n; ++i) {
        float diff = f1[i] - f2[i];
        dist += diff * diff;
    }
    return std::sqrt(dist);
}

/**
 * TrainSystem: Implements OPF training for entities with CFeatures and CLabel.
 * 
 * Input Requirements:
 *   - Entity must have CFeatures (feature vectors)
 *   - Entity must have CLabel (training labels)
 *   - Entity should have CSamples (optional, for sample subset tracking)
 * 
 * Output:
 *   - Adds CModelParams: trained model with prototypes, path values, predecessors
 *   - Adds CEvalMetrics: training statistics (num prototypes, etc.)
 * 
 * Algorithm:
 *   1. Prototype Selection: Simple heuristic (select one representative per label)
 *   2. IFT (Image Foresting Transform): Propagate labels via minimum cost paths
 *   3. Result: Each node assigned a label via its nearest prototype
 */
class TrainSystem : public ISystem {
public:
    TrainSystem() = default;
    virtual ~TrainSystem() = default;

    void update(EntityRegistry& registry, double dt) override {
        auto start_time = std::chrono::high_resolution_clock::now();

        // Find all training entities (those with CFeatures and CLabel)
        auto training_entities = registry.view<CFeatures, CLabel>();

        for (auto entity_id : training_entities) {
            train_entity(registry, entity_id);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    }

private:
    /**
     * Train a single entity (Subgraph) using OPF training algorithm.
     */
    void train_entity(EntityRegistry& registry, EntityId entity_id) {
        auto* features = registry.getComponent<CFeatures>(entity_id);
        auto* label = registry.getComponent<CLabel>(entity_id);

        if (!features || !label) return;

        // Determine number of samples and features
        // For a single-entity training, we assume:
        // - features->values contains all sample features concatenated
        // - We need metadata about how many samples and features per sample
        // For now, we'll work with feature vectors from a sample list

        auto* samples = registry.getComponent<CSamples>(entity_id);
        if (!samples) {
            // Single sample case
            train_single_sample(registry, entity_id, features, label);
            return;
        }

        // Multi-sample training case
        train_subgraph(registry, entity_id, samples, features, label);
    }

    /**
     * Train a single sample entity (creates trivial model).
     */
    void train_single_sample(EntityRegistry& registry, EntityId entity_id,
                             CFeatures* features, CLabel* label) {
        // Create model with single prototype
        auto model = std::make_unique<CModelParams>(1, 1);
        model->prototypes = {0};
        model->node_labels = {label->label};
        model->pathvalues = {0.0f};
        model->predecessors = {-1};
        model->ordered_nodes = {0};

        registry.addComponent<CModelParams>(entity_id, std::move(*model));

        auto metrics = std::make_unique<CEvalMetrics>();
        metrics->num_prototypes = 1;
        metrics->num_samples_trained = 1;
        metrics->accuracy = 1.0f;  // Single sample is always correct
        metrics->status = "trained";
        registry.addComponent<CEvalMetrics>(entity_id, std::move(*metrics));
    }

    /**
     * Train a subgraph with multiple samples.
     * 
     * This is a simplified version of opf_OPFTraining:
     * 1. Select prototypes (one per label)
     * 2. Run IFT to propagate labels
     * 3. Store model parameters
     */
    void train_subgraph(EntityRegistry& registry, EntityId entity_id,
                        CSamples* samples, CFeatures* features, CLabel* primary_label) {
        
        int num_samples = samples->indices.size();
        if (num_samples == 0) return;

        // For simplicity in this prototype phase, we'll store feature dimension
        // In a real implementation, we'd have a separate component storing sample metadata
        int nfeats = 1;  // Default; could come from component metadata

        auto model = std::make_unique<CModelParams>(nfeats, num_samples);
        
        // ====== Step 1: Prototype Selection ======
        // Simplified: select first sample as prototype for its label
        // In full OPF, this would use MST (Minimum Spanning Tree)
        std::vector<int> prototypes;
        std::vector<int> prototype_labels;
        
        prototypes.push_back(0);
        prototype_labels.push_back(primary_label->label);
        
        model->prototypes = prototypes;

        // ====== Step 2: IFT (Image Foresting Transform) ======
        // Simplified IFT: assign each sample its prototype's label
        std::vector<float> pathvals(num_samples, std::numeric_limits<float>::max());
        std::vector<int> preds(num_samples, -1);
        std::vector<int> labels(num_samples, -1);

        // Initialize prototypes
        for (int proto_idx : prototypes) {
            pathvals[proto_idx] = 0.0f;
            labels[proto_idx] = primary_label->label;
            preds[proto_idx] = -1;
        }

        // Simplified propagation: nearest neighbor assignment
        for (int i = 0; i < num_samples; ++i) {
            if (pathvals[i] == 0.0f) continue;  // Skip prototypes

            float best_cost = std::numeric_limits<float>::max();
            int best_pred = -1;
            int best_label = -1;

            // Find nearest prototype
            for (int proto_idx : prototypes) {
                // Cost = prototype's path value + distance to current sample
                // Simplified: just use distance
                float cost = pathvals[proto_idx];  // Start with proto's path value
                
                if (cost < best_cost) {
                    best_cost = cost;
                    best_pred = proto_idx;
                    best_label = labels[proto_idx];
                }
            }

            if (best_pred != -1) {
                pathvals[i] = best_cost;
                preds[i] = best_pred;
                labels[i] = best_label;
            }
        }

        // ====== Step 3: Order nodes by path value ======
        std::vector<int> ordered_nodes(num_samples);
        std::iota(ordered_nodes.begin(), ordered_nodes.end(), 0);
        std::sort(ordered_nodes.begin(), ordered_nodes.end(),
                  [&pathvals](int a, int b) { return pathvals[a] < pathvals[b]; });

        // ====== Store Results ======
        model->pathvalues = pathvals;
        model->predecessors = preds;
        model->node_labels = labels;
        model->ordered_nodes = ordered_nodes;

        registry.addComponent<CModelParams>(entity_id, std::move(*model));

        // ====== Create Metrics ======
        auto metrics = std::make_unique<CEvalMetrics>();
        metrics->num_prototypes = prototypes.size();
        metrics->num_samples_trained = num_samples;
        metrics->status = "trained";
        registry.addComponent<CEvalMetrics>(entity_id, std::move(*metrics));
    }
};

}  // namespace ecs
