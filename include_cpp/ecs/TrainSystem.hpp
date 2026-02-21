#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <numeric>
#include <limits>
#include <chrono>
#include <queue>
#include "System.h"
#include "EntityRegistry.h"
#include "Components.hpp"
#include "ClassifySystem.hpp"

namespace ecs {

/**
 * TrainSystem: Implements full OPF training with MST prototype selection and IFT.
 * 
 * Workflow (based on test_example3.cpp analysis):
 *   1. MST Prototype Selection: Prim's algorithm to find class boundary samples
 *   2. IFT: Image Foresting Transform with max(pathval, weight) cost function
 *   3. Ordering: Nodes sorted by pathval for efficient classification
 * 
 * Input: Entities with CFeatures (multi-sample via CSamples) and CLabel
 * Output: CModelParams (prototypes, pathvals, predecessors, ordered_nodes) + CEvalMetrics
 */
class TrainSystem : public ISystem {
public:
    TrainSystem() = default;
    virtual ~TrainSystem() = default;

    void update(EntityRegistry& registry, double dt) override {
        // First, process all subgraph entities with CSamples
        auto subgraph_entities = registry.view<CSamples>();
        for (auto entity_id : subgraph_entities) {
            train_entity(registry, entity_id);
        }

        // Then, process single-sample entities (features + label) that are not subgraphs
        auto training_entities = registry.view<CFeatures, CLabel>();
        for (auto entity_id : training_entities) {
            if (registry.hasComponent<CSamples>(entity_id)) continue;
            train_entity(registry, entity_id);
        }
    }

private:
    void train_entity(EntityRegistry& registry, EntityId entity_id) {
        auto* samples = registry.getComponent<CSamples>(entity_id);
        if (samples) {
            train_subgraph_mst_ift(registry, entity_id, samples);
            return;
        }

        auto* features = registry.getComponent<CFeatures>(entity_id);
        auto* label = registry.getComponent<CLabel>(entity_id);
        if (!features || !label) return;

        train_single_sample(registry, entity_id, features, label);
    }

    void train_single_sample(EntityRegistry& registry, EntityId entity_id,
                             CFeatures* features, CLabel* label) {
        auto model = std::make_unique<CModelParams>(features->values.size(), 1);
        model->prototypes = {0};
        model->node_labels = {label->label};
        model->pathvalues = {0.0f};
        model->predecessors = {-1};
        model->ordered_nodes = {0};
        model->prototype_features = {features->values};
        model->all_features = {features->values};

        registry.addComponent<CModelParams>(entity_id, std::move(*model));

        auto metrics = std::make_unique<CEvalMetrics>();
        metrics->num_prototypes = 1;
        metrics->num_samples_trained = 1;
        metrics->accuracy = 1.0f;
        metrics->status = "trained";
        registry.addComponent<CEvalMetrics>(entity_id, std::move(*metrics));
    }

    /**
     * Full OPF Training: MST Prototype Selection + IFT
     * 
     * Implements algorithms documented in ecs.md Section 14
     */
    void train_subgraph_mst_ift(EntityRegistry& registry, EntityId entity_id,
                                CSamples* samples) {
        // Extract sample features from registry
        std::vector<std::vector<float>> sample_features;
        std::vector<int> sample_labels;
        
        for (int sample_id : samples->indices) {
            auto* feat = registry.getComponent<CFeatures>(sample_id);
            auto* lbl = registry.getComponent<CLabel>(sample_id);
            if (feat && lbl) {
                sample_features.push_back(feat->values);
                sample_labels.push_back(lbl->label);
            }
        }

        int num_samples = sample_features.size();
        if (num_samples == 0) return;

        int nfeats = sample_features[0].size();
        auto model = std::make_unique<CModelParams>(nfeats, num_samples);
        model->all_features = sample_features;  // Store for classification

        // ====== Phase A: MST Prototype Selection (Prim's Algorithm) ======
        std::vector<float> mst_pathval(num_samples, std::numeric_limits<float>::infinity());
        std::vector<int> mst_pred(num_samples, -1);
        std::vector<bool> visited(num_samples, false);
        std::vector<int> status(num_samples, 0);  // 0=normal, 1=prototype

        // Min-heap for Prim's: (distance, node_id)
        using PQElement = std::pair<float, int>;
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;

        // Start MST from node 0
        mst_pathval[0] = 0.0f;
        pq.push({0.0f, 0});

        while (!pq.empty()) {
            auto [cost, p] = pq.top();
            pq.pop();

            if (visited[p]) continue;
            visited[p] = true;

            // Detect prototype: class boundary edge
            if (mst_pred[p] != -1 && sample_labels[p] != sample_labels[mst_pred[p]]) {
                status[p] = 1;
                status[mst_pred[p]] = 1;
            }

            // Expand to all unvisited neighbors
            for (int q = 0; q < num_samples; ++q) {
                if (visited[q]) continue;

                float weight = euclidean_distance(sample_features[p], sample_features[q]);
                if (weight < mst_pathval[q]) {
                    mst_pathval[q] = weight;
                    mst_pred[q] = p;
                    pq.push({weight, q});
                }
            }
        }

        // Collect prototypes
        std::vector<int> prototypes;
        for (int i = 0; i < num_samples; ++i) {
            if (status[i] == 1) {
                prototypes.push_back(i);
            }
        }
        // Ensure at least one prototype
        if (prototypes.empty()) {
            prototypes.push_back(0);
            status[0] = 1;
        }

        model->prototypes = prototypes;

        // ====== Phase B: IFT (Image Foresting Transform) ======
        std::vector<float> pathvals(num_samples, std::numeric_limits<float>::infinity());
        std::vector<int> preds(num_samples, -1);
        std::vector<int> labels(num_samples, -1);

        // Priority queue for IFT: (cost, node_id)
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> ift_pq;

        // Initialize prototypes
        for (int proto_idx : prototypes) {
            pathvals[proto_idx] = 0.0f;
            labels[proto_idx] = sample_labels[proto_idx];
            preds[proto_idx] = -1;
            ift_pq.push({0.0f, proto_idx});
        }

        // IFT propagation with max(pathval, weight) cost function
        std::vector<int> ordered_nodes;
        while (!ift_pq.empty()) {
            auto [curr_cost, p] = ift_pq.top();
            ift_pq.pop();

            if (curr_cost > pathvals[p]) continue;  // Already processed

            ordered_nodes.push_back(p);  // Add in pathval order

            // Propagate to all neighbors
            for (int q = 0; q < num_samples; ++q) {
                if (pathvals[p] < pathvals[q]) {
                    float weight = euclidean_distance(sample_features[p], sample_features[q]);
                    float cost = std::max(pathvals[p], weight);  // OPF cost function

                    if (cost < pathvals[q]) {
                        pathvals[q] = cost;
                        preds[q] = p;
                        labels[q] = labels[p];
                        ift_pq.push({cost, q});
                    }
                }
            }
        }

        // ====== Store Results ======
        model->pathvalues = pathvals;
        model->predecessors = preds;
        model->node_labels = labels;
        model->ordered_nodes = ordered_nodes;

        // Store prototype features for classification
        model->prototype_features.clear();
        for (int proto_idx : prototypes) {
            model->prototype_features.push_back(sample_features[proto_idx]);
        }

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
