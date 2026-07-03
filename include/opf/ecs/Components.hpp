#include <vector>
#include <cstdint>
#include <memory>
#include <cmath>
#include "Datatypes.hpp"

class IComponent {
public:
    virtual ~IComponent() = default;
};

// Componente Puro de Topologia
struct TopologyComponent {
    // Vetorizado: Acessado diretamente por ID do Nó (Entity ID)
    std::vector<uint32_t> adjacency_offset; // Onde começam os vizinhos do nó i
    std::vector<uint32_t> neighbor_count;    // Quantidade de vizinhos (k) do nó i
    std::vector<float>    distance_factor;   // d_f (peso máximo de arco no k-nn) do nó i

    // Buffer plano e contíguo contendo os IDs de todos os vizinhos
    std::vector<uint32_t> adjacency_list;
    
    // VETOR DE PESOS PARALELO: O peso da aresta para o vizinho correspondente
    // Se adjacency_list[5] é o nó V, edge_weights[5] é o peso da aresta para V.
    std::vector<float>    edge_weights;    
};


// Componente de Features: Armazena os dados brutos e metadados de navegação
template<typename T = float>
requires NumericBuffer<T>
struct FeatureComponent : public IComponent {
    std::vector<T> raw_data;
    size_t num_features = 0;
    size_t stride = 0;
    size_t offset = 0;
};

// Componente de Atributos: Armazena atributos complexos (ex: Texturas)
template<typename T = float>
requires AttributeBuffer<T>
struct AttributeComponent : public IComponent {
    std::vector<T> attributes;
    size_t num_attributes = 0;
    size_t stride = 0;
    size_t offset = 0;
};

// Componente de Clustering: Armazena o estado do algoritmo de clustering
struct ClusteringComponent : public IComponent {
    std::vector<TopologyComponent> topology;
    std::vector<float> density;
    std::vector<float> max_dist;
    std::vector<uint32_t> label;
    std::vector<uint32_t> parent;
    std::vector<uint32_t> label_count;
    std::vector<uint32_t> label_size;
};


template<typename T>
struct NodeComponent : public IComponent {
    uint32_t id;
    AttributeComponent<T> attribute;
};


// Representação lógica no motor de execução do OPF++
struct EntityGraphView {
    const TopologyComponent& topology;
    
    // Composição por associação: múltiplos experts/kernels sem duplicação de dados
    const FeatureComponent<float>& spatial_features;   // K_rest (ex: Stride tradicional)
    const FeatureComponent<float>& appearance_features;// K_new  (ex: Strided/Planar)
};