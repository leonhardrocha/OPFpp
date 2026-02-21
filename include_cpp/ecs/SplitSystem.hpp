#pragma once

#include <cmath>
#include <algorithm>
#include "System.h"
#include "Components.hpp"

namespace ecs {

class SplitSystem : public ISystem {
public:
    void update(EntityRegistry &registry, double dt) override {
        // Find Dataset entities with CIOPath
        auto datasets = registry.view<CIOPath>();
        
        for (auto dataset_id : datasets) {
            auto *path = registry.getComponent<CIOPath>(dataset_id);
            if (!path) continue;
            
            // Get split parameters if they exist, otherwise use defaults
            auto *params = registry.getComponent<CSplitParams>(dataset_id);
            float train_pct = params ? params->train_percentage : 0.5f;
            float eval_pct = params ? params->eval_percentage : 0.0f;
            float test_pct = params ? params->test_percentage : 0.5f;
            
            // Normalize percentages
            float total = train_pct + eval_pct + test_pct;
            
            train_pct /= total;
            eval_pct /= total;
            test_pct /= total;
            
            total = train_pct + eval_pct + test_pct;
            if (total > 1.0f + std::numeric_limits<float>::epsilon() || total < 1.0f - std::numeric_limits<float>::epsilon()) continue;  // Skip invalid split
            
            // Generate sample count (mock: use 100 samples for demo)
            int total_samples = 100;
            
            // Calculate split indices
            int train_count = static_cast<int>(total_samples * train_pct);
            int eval_count = static_cast<int>(total_samples * eval_pct);
            int test_count = total_samples - train_count - eval_count;
            
            // Create training subgraph
            auto train_sg = registry.create(EntityType::Subgraph);
            std::vector<int> train_indices;
            for (int i = 0; i < train_count; ++i) train_indices.push_back(i);
            registry.addComponent<CSamples>(train_sg.id, CSamples(train_indices));
            registry.addComponent<CFlags>(train_sg.id, CFlags(true, false, false));
            
            // Create evaluating subgraph (if eval_count > 0)
            if (eval_count > 0) {
                auto eval_sg = registry.create(EntityType::Subgraph);
                std::vector<int> eval_indices;
                for (int i = 0; i < eval_count; ++i) 
                    eval_indices.push_back(train_count + i);
                registry.addComponent<CSamples>(eval_sg.id, CSamples(eval_indices));
                registry.addComponent<CFlags>(eval_sg.id, CFlags(false, true, false));
            }
            
            // Create testing subgraph
            auto test_sg = registry.create(EntityType::Subgraph);
            std::vector<int> test_indices;
            for (int i = 0; i < test_count; ++i)
                test_indices.push_back(train_count + eval_count + i);
            registry.addComponent<CSamples>(test_sg.id, CSamples(test_indices));
            registry.addComponent<CFlags>(test_sg.id, CFlags(false, false, true));
            
            // Mark dataset as split
            registry.addComponent<CFlags>(dataset_id, CFlags(false, false, false));
        }
    }
};

} // namespace ecs
