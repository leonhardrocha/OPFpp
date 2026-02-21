#pragma once

#include "System.h"
#include "EntityRegistry.h"
#include "Components.hpp"

namespace ecs {

/**
 * AccuracySystem: Computes accuracy based on predicted vs true labels.
 *
 * Modes:
 *  - Subgraph evaluation: entity with CSamples; accuracy computed over sample entities.
 *  - Single-entity evaluation: entity with CLabel + CTrueLabel.
 */
class AccuracySystem : public ISystem {
public:
    AccuracySystem() = default;
    ~AccuracySystem() override = default;

    void update(EntityRegistry &registry, double /*dt*/) override {
        // Evaluate subgraphs first
        auto subgraphs = registry.view<CSamples>();
        for (auto entity_id : subgraphs) {
            evaluate_subgraph(registry, entity_id);
        }

        // Evaluate standalone entities (not part of a CSamples set)
        auto samples = registry.view<CLabel, CTrueLabel>();
        for (auto entity_id : samples) {
            if (registry.hasComponent<CSamples>(entity_id)) {
                continue;
            }
            evaluate_single(registry, entity_id);
        }
    }

private:
    void evaluate_single(EntityRegistry &registry, EntityId entity_id) {
        auto *pred = registry.getComponent<CLabel>(entity_id);
        auto *truth = registry.getComponent<CTrueLabel>(entity_id);
        if (!pred || !truth) return;

        int correct = (pred->label == truth->label) ? 1 : 0;
        int total = 1;

        auto *metrics = registry.getComponent<CEvalMetrics>(entity_id);
        if (!metrics) {
            registry.addComponent<CEvalMetrics>(entity_id, CEvalMetrics());
            metrics = registry.getComponent<CEvalMetrics>(entity_id);
        }

        if (metrics) {
            metrics->correct_classifications = correct;
            metrics->total_classifications = total;
            metrics->accuracy = total > 0 ? static_cast<float>(correct) / total : 0.0f;
            metrics->status = "evaluated";
        }
    }

    void evaluate_subgraph(EntityRegistry &registry, EntityId entity_id) {
        auto *samples = registry.getComponent<CSamples>(entity_id);
        if (!samples) return;

        int correct = 0;
        int total = 0;

        for (int sample_id : samples->indices) {
            auto *pred = registry.getComponent<CLabel>(sample_id);
            auto *truth = registry.getComponent<CTrueLabel>(sample_id);
            if (!pred || !truth) continue;

            total++;
            if (pred->label == truth->label) {
                correct++;
            }
        }

        auto *metrics = registry.getComponent<CEvalMetrics>(entity_id);
        if (!metrics) {
            registry.addComponent<CEvalMetrics>(entity_id, CEvalMetrics());
            metrics = registry.getComponent<CEvalMetrics>(entity_id);
        }

        if (metrics) {
            metrics->correct_classifications = correct;
            metrics->total_classifications = total;
            metrics->accuracy = total > 0 ? static_cast<float>(correct) / total : 0.0f;
            metrics->status = "evaluated";
        }
    }
};

} // namespace ecs
