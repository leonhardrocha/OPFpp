#include "Components.hpp"
#include <memory>

class GraphBuilderSystem {
public:
    // O Sistema cuida da criação e alocação do componente puro
    static std::unique_ptr<FeatureComponent> create_features(
        size_t total_nodes, size_t num_features, size_t custom_stride, size_t custom_offset) 
    {
        auto feature_component = std::make_unique<FeatureComponent>();
        feature_component->num_features = num_features;
        feature_component->stride = custom_stride > 0 ? custom_stride : num_features;
        feature_component->offset = custom_offset;
        
        // Aloca o buffer contíguo
        feature_component->raw_data.resize(total_nodes * feature_component->stride, 0.0f);
        return feature_component;
    }
};