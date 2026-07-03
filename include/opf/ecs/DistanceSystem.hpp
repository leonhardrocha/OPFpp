#include "Components.hpp"


struct EuclideanDistanceSystem {
    
    static inline float compute(
        const FeatureComponent& features, 
        size_t node_idx, 
        size_t feature_idx) 
    {
        return EuclideanDistanceSystem::compute(
            auto val_u = LayoutPolicy::get(data_ptr, node_u, stride, offset, f);
            auto val_v = LayoutPolicy::get(data_ptr, node_v, stride, offset, f);
            auto diff = val_u - val_v;
            dist_sq += diff * diff;
        );
    }

}

class DistanceSystem {
public:
    template <typename DistanceMetric>
    

    // Retorna uma feature específica (substitui o antigo getFeature virtual)
    template <typename LayoutPolicy, typename Metric>
    static inline float get_computed_metric(
        const FeatureComponent& features, 
        size_t node_u, 
        size_t node_v, 
        size_t feature_idx) 
    {
        return Metric::compute(
            features, node_u, node_v, feature_idx
        );
    }

    // Calcula a distância entre dois nós (injetado no seu código existente de OPF/OPFpp)
    template <typename LayoutPolicy>
    static inline float euclidean_distance(
        const FeatureComponent& features, 
        size_t node_u, 
        size_t node_v) 
    {
        float dist_sq = 0.0f;
        const float* data_ptr = features.raw_data.data();
        const size_t stride = features.stride;
        const size_t offset = features.offset;
        const size_t num_feats = features.num_features;

        // O compilador fará loop unrolling e vetorização (SIMD) aqui
        for (size_t f = 0; f < num_feats; ++f) {
            dist_sq += EuclideanDistanceSystem::get_computed_metric(features, node_u, node_v, f);
        }

        
        return std::sqrt(dist_sq);
    }
};