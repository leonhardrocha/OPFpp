#include "GraphSystem.hpp"
#include "DistanceSystem.hpp"
#include "Components.hpp"
#include "Layouts.hpp"

enum class OPFExecutionSystemLayoutType {
    INTERLEAVED, // RGBRGBRGB
    PLANAR       // RRRGGGBBB
};

class OPFExecutionSystem {
public:
    // Esta função recebe a decisão em tempo de execução e injeta o template correto
    static void run_clustering_loop(
        OPFExecutionSystemLayoutType layout_type,
        const TopologyComponent& topo,
        const FeatureComponent& features) 
    {
        switch (layout_type) {
            case OPFExecutionSystemLayoutType::INTERLEAVED:
                execute_internal_loop<InterleavedLayout>(topo, features);
                break;
            case OPFExecutionSystemLayoutType::PLANAR:
                execute_internal_loop<PlanarLayout>(topo, features);
                break;
        }
    }

private:
    // O seu algoritmo real (fila, propagação de rótulos) fica aqui, 
    // mas agora está fortemente tipado com o    Layout para máxima performance.
    template <typename LayoutPolicy>
    static void execute_internal_loop(
        const TopologyComponent& topo,
        const FeatureComponent& features) 
    {
        // 1. Extração dos ponteiros brutos para garantir que o compilador enxergue a contiguidade
        const float* raw_features = features.raw_data.data();
        const size_t stride = features.stride;
        const size_t offset = features.offset;
        const size_t num_feats = features.num_features;

        // Seus loops existentes de clustering (ex: busca de densidade ou propagação IFT)
        for (size_t u = 0; u < topo.nodes.size(); ++u) {
            // ... sua lógica de fila de prioridade ...
            
            // Chamada da métrica otimizada para o Layout atual
            float dist = DistanceSystem::euclidean_distance<LayoutPolicy>(
                raw_features, u, v, stride, offset, num_feats
            );
            
            // ... computação de f_max / f_min baseado no isomorfismo do gargalo ...
        }
    }
};