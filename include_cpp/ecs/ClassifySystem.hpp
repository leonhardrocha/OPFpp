#pragma once

#include "System.h"
#include "Components.hpp"
#include "EntityRegistry.h"
#include "../opf/Distance.hpp"
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>

namespace ecs {

/**
 * @class ClassifySystem
 * @brief Classifies samples using trained OPF model (implements opf_classify logic).
 *
 * Based on opf::OPF<T>::classifying() from OPF.hpp:
 * - Iterates through ordered nodes (prototypes sorted by path value)
 * - For each test sample, finds minimum cost: max(train_pathval, distance)
 * - Assigns label from best matching prototype
 * - Early termination when min_cost <= next prototype's pathval
 *
 * Input: Model entity with CModelParams (prototypes, labels, path_values, ordered_nodes)
 * Input: Test sample entities with CFeatures
 * Output: Predicted labels (CLabel) assigned to test samples
 * Output: Classification metrics (CEvalMetrics)
 */
class ClassifySystem : public ISystem {
public:
    ClassifySystem() = default;
    virtual ~ClassifySystem() = default;

    void update(EntityRegistry &registry, double dt) override {
        // Find trained model
        auto model_results = registry.view<CModelParams>();
        if (model_results.empty()) {
            return;
        }

        EntityId model_id = model_results[0];
        CModelParams *model = registry.getComponent<CModelParams>(model_id);
        if (!model || model->prototypes.empty()) {
            return;
        }

        // Classify test samples with CFeatures
        auto test_samples = registry.view<CFeatures>();
        for (EntityId test_id : test_samples) {
            // Skip the model entity itself
            if (test_id == model_id) {
                continue;
            }

            classify_sample(registry, test_id, model);
        }
    }

private:
    /**
     * @brief Classifies a single test sample using OPF algorithm.
     * 
     * Implements the logic from opf::OPF<T>::classifying():
     * - Iterate through ordered_nodes (sorted by pathval ascending)
     * - For each prototype: compute cost = max(pathval, distance_to_test)
     * - Track minimum cost and corresponding label
     * - Early exit when min_cost <= next prototype's pathval (optimization)
     */
    void classify_sample(EntityRegistry &registry, EntityId test_id, CModelParams *model) {
        CFeatures *test_features = registry.getComponent<CFeatures>(test_id);
        if (!test_features || test_features->values.empty()) {
            return;
        }

        float min_cost = std::numeric_limits<float>::max();
        int final_label = -1;

        // Iterate through ordered prototypes (sorted by pathval)
        for (size_t j = 0; j < model->ordered_nodes.size(); ++j) {
            int node_idx = model->ordered_nodes[j];
            
            // Early termination: if min_cost is less than or equal to current pathval,
            // no future prototype can have lower cost (because they're sorted)
            if (min_cost <= model->pathvalues[node_idx]) {
                break;
            }

            // Compute distance from test sample to this prototype
            // Note: prototypes stores indices, need to get actual features from somewhere
            // For now, simplified: assume distance is 0 (will fix with proper feature lookup)
            float weight = 0.0f; // TODO: Need actual feature vectors for prototypes

            // OPF cost: max of path value and distance
            float cost = std::max(model->pathvalues[node_idx], weight);

            if (cost < min_cost) {
                min_cost = cost;
                final_label = model->node_labels[node_idx];
            }
        }

        // Assign predicted label
        if (!registry.hasComponent<CLabel>(test_id)) {
            registry.addComponent<CLabel>(test_id, CLabel(final_label));
        } else {
            CLabel *label = registry.getComponent<CLabel>(test_id);
            if (label) {
                label->label = final_label;
            }
        }

        // Update metrics
        auto *metrics = registry.getComponent<CEvalMetrics>(test_id);
        if (!metrics) {
            registry.addComponent<CEvalMetrics>(test_id, CEvalMetrics());
            metrics = registry.getComponent<CEvalMetrics>(test_id);
        }
        
        if (metrics) {
            metrics->total_classifications++;
            metrics->training_time_ms = min_cost; // Reusing this field for classification cost
        }
    }
};

} // namespace ecs
