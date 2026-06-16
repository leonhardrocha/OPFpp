#ifndef OPF_KERNEL_SUBGRAPH_HPP
#define OPF_KERNEL_SUBGRAPH_HPP

/// KernelSubGraph.hpp — copy-on-write decorator over an existing Subgraph.
///
/// Design goals
/// ------------
/// * All scalar node metadata (truelabel, label, position, pathval, pred,
///   status, relevant, root, dens, radius, nplatadj) is **shallow** — reads
///   go back to the source Node via pointer; writes copy-on-write into a
///   local overlay.
/// * The adjacency list is also shallow (COW): the source adj vector is
///   referenced by a shared_ptr; the first in-place mutation triggers a deep
///   copy of that adj vector.
/// * The feature vector is **always deep-copied** but only for the slice
///   [feat_start, feat_end) assigned to this kernel.
///
/// Usage
/// -----
///   auto kernels = opf::splitSubgraphIntoKernels(sg, 4);
///   // kernels[i] is a KernelSubGraph<T> that can be passed anywhere a
///   // const Subgraph<T>& is accepted via an implicit operator Subgraph<T>().
///
/// Thread safety
/// -------------
/// Each KernelSubGraph owns its own feature slices, so concurrent read access
/// to different kernels is safe.  The COW adj is NOT thread-safe for
/// concurrent writes to the same kernel.

#include <opf/Subgraph.hpp>
#include <cmath>       // std::ceil
#include <stdexcept>
#include <vector>
#include <memory>
#include <algorithm>
#include <cassert>

namespace opf {

    /// Decorator over an existing Subgraph<T> for a single feature-slice kernel.
    ///
    /// * Graph-level metadata (df, K, mindens, maxdens, bestk, nlabels) is
    ///   shallow by default (reads go to the source); writes go to a local copy.
    /// * kernel_feature_sizes and kernel_weights are local (not delegated).
    /// * Node proxies are created lazily and cached.

    template<typename T>
    class KernelSubGraph {
    public:
        /// Construct a kernel covering features [feat_start, feat_end) of *source*.
       KernelSubGraph(Subgraph<T>& source, int feat_start, int feat_end)
        : source_(source)
        , feat_start_(feat_start)
        , feat_end_(feat_end)
        , nfeats_(feat_end - feat_start)
    {
        assert(feat_start >= 0);
        assert(feat_end > feat_start);
        assert(feat_end <= source.getNumFeats());

        const int n = source.getNumNodes();
        nodes_.reserve(n);
        
        for (int i = 0; i < n; ++i) {
            const Node<T>& src_node = source.getNode(i);
            
            // 1. Cria um Node nativo padrão usando o construtor de 0 argumentos
            nodes_.emplace_back(); 
            Node<T>& local_node = nodes_.back();

            // 2. Deep-copy do slice de features alocado dinamicamente
            const auto& full_feat = *src_node.getFeat();
            auto slice = std::make_shared<std::vector<T>>(
                full_feat.begin() + feat_start_,
                full_feat.begin() + feat_end_
            );
            local_node.setFeat(std::move(slice));

            // 3. Copia a lista de adjacência base original para o nó local
            local_node.setAdj(src_node.getAdj());

            // 4. Preserva e clona todos os metadados escalares do nó pai original
            local_node.setPosition(src_node.getPosition());
            local_node.setTruelabel(src_node.getTruelabel());
            local_node.setLabel(src_node.getLabel());
            local_node.setDens(src_node.getDens());
            local_node.setRadius(src_node.getRadius());
            local_node.setPathval(src_node.getPathval());
            local_node.setPred(src_node.getPred());
            local_node.setRoot(src_node.getRoot());
            local_node.setStatus(src_node.getStatus());
            local_node.setRelevant(src_node.getRelevant());
            local_node.setNplatadj(src_node.getNplatadj());
        }
        
        // 5. Inicializa os ponteiros de overlays centralizados como limpos (nullptr)
        local_adj_overlays_.resize(n, nullptr);
    }

        // --- Subgraph-level getters (delegate to source unless locally overridden) ---
        int   getNumNodes()  const { return static_cast<int>(nodes_.size()); }
        int   getNumFeats()  const { return nfeats_; }
        int   getBestK()     const { return has_bestk_   ? local_bestk_   : source_.getBestK(); }
        int   getNumLabels() const { return has_nlabels_ ? local_nlabels_ : source_.getNumLabels(); }
        float getDf()        const { return has_df_      ? local_df_      : source_.getDf(); }
        float getMinDens()   const { return has_mindens_ ? local_mindens_ : source_.getMinDens(); }
        float getMaxDens()   const { return has_maxdens_ ? local_maxdens_ : source_.getMaxDens(); }
        float getK()         const { return has_K_       ? local_K_       : source_.getK(); }

        const std::vector<int>&   getKernelFeatureSizes() const { return kernel_feature_sizes_; }
        const std::vector<float>& getKernelWeights()      const { return kernel_weights_; }

        // --- Subgraph-level setters (local only — do not propagate to source) ---
        void setNumFeats(int v)                                   { nfeats_        = v; }
        void setBestK(int v)    { has_bestk_   = true; local_bestk_   = v; }
        void setNumLabels(int v){ has_nlabels_ = true; local_nlabels_ = v; }
        void setDf(float v)     { has_df_      = true; local_df_      = v; }
        void setMinDens(float v){ has_mindens_ = true; local_mindens_ = v; }
        void setMaxDens(float v){ has_maxdens_ = true; local_maxdens_ = v; }
        void setK(float v)      { has_K_       = true; local_K_       = v; }
        void setKernelFeatureSizes(const std::vector<int>&   v) { kernel_feature_sizes_ = v; }
        void setKernelWeights(const std::vector<float>& v)      { kernel_weights_       = v; }

        // --- Node access ---
        const Node<T>& getNode(int i) const { return nodes_[i]; }
            Node<T>& getNode(int i)       { return nodes_[i]; }
        
        // Obtém a adjacência CoW atual (Usado pelo Node)
        std::shared_ptr<const std::vector<int>> getNodeSharedAdj(int node_idx) const {
            return local_adj_overlays_[node_idx];
        }
        
        // 3. Método mutador usado pelo laço de distribuição em OPFpp.hpp
        void setNodeSharedAdj(int node_idx, std::shared_ptr<const std::vector<int>> shared_adj) {
            #ifndef NDEBUG
            if (node_idx < 0 || node_idx >= this->getNumNodes()) {
                throw std::out_of_range("Índice de nó fora dos limites.");
            }
            #endif
            this->local_adj_overlays_[node_idx] = shared_adj;
        }

        // 4. Método de leitura limpo para C++ obter a adjacência real do Kernel
        const std::vector<int>& getKernelAdj(int node_idx) const {
            if (local_adj_overlays_[node_idx] != nullptr) {
                return *(local_adj_overlays_[node_idx]);
            }
            // Se não houver overlay, retorna a adjacência original do nó do Subgraph
            return this->getNode(node_idx).getAdj(); 
        }

        /// Compute per-node modular probability from density: prob = ln(dens)
        /// with zero-guard (dens <= epsilon -> prob = 0).
        std::vector<float> computeLogDensityProbabilities(float epsilon = 1e-12f) const {
            std::vector<float> probs(getNumNodes(), 0.0f);
            for (int i = 0; i < getNumNodes(); ++i) {
                float dens = nodes_[i].getDens();
                if (dens > epsilon) {
                    float p = std::log(dens);
                    probs[i] = std::isfinite(p) ? p : 0.0f;
                }
            }
            return probs;
        }

        /// Convert to a plain Subgraph<T> (full deep copy — use sparingly).
        Subgraph<T> toSubgraph() const {
            const int n = getNumNodes();
            Subgraph<T> out(n);
            out.setNumFeats(nfeats_);
            out.setNumLabels(getNumLabels());
            out.setBestK(getBestK());
            out.setDf(getDf());
            out.setK(getK());
            out.setMinDens(getMinDens());
            out.setMaxDens(getMaxDens());
            for (int i = 0; i < n; ++i) {
                const auto& kn = nodes_[i];
                Node<T>& dst = out.getNode(i);
                dst.setTruelabel(kn.getTruelabel());
                dst.setLabel(kn.getLabel());
                dst.setPosition(kn.getPosition());
                dst.setPathval(kn.getPathval());
                dst.setPred(kn.getPred());
                dst.setStatus(kn.getStatus());
                dst.setRelevant(kn.getRelevant());
                dst.setRoot(kn.getRoot());
                dst.setFeat(kn.getFeat());
                dst.getAdj() = kn.getAdj();
            }
            return out;
        }

        void clearOrderedListOfNodes() { ordered_list_of_nodes_.clear(); }
        void addOrderedNode(int p) { ordered_list_of_nodes_.push_back(p); }
        const std::vector<int>& getOrderedListOfNodes() const { return ordered_list_of_nodes_; }
        int featStart() const { return feat_start_; }
        int featEnd()   const { return feat_end_;   }

    private:
        Subgraph<T>& source_;
        int feat_start_;
        int feat_end_;
        int nfeats_;

        std::vector<Node<T>> nodes_;
        std::vector<int> ordered_list_of_nodes_;

        // Local COW scalar fields
        bool has_bestk_   = false; int   local_bestk_   = 0;
        bool has_nlabels_ = false; int   local_nlabels_ = 0;
        bool has_df_      = false; float local_df_      = 0.f;
        bool has_mindens_ = false; float local_mindens_ = 0.f;
        bool has_maxdens_ = false; float local_maxdens_ = 0.f;
        bool has_K_       = false; float local_K_       = 0.f;

        std::vector<int>   kernel_feature_sizes_;
        std::vector<float> kernel_weights_;
        // O vetor centralizado que gerencia os overlays de adjacência de cada nó do Kernel
        std::vector<std::shared_ptr<const std::vector<int>>> local_adj_overlays_;    
    };

    /// Split *sg* into *n_kernels* KernelSubGraph objects by partitioning features.
    ///
    ///   slice_size = ceil(nfeats / n_kernels)
    ///   kernel k covers features [k*slice_size, min((k+1)*slice_size, nfeats))
    ///
    /// The last kernel may receive fewer features (remainder).  Returns fewer than
    /// *n_kernels* entries only when nfeats < n_kernels.
    /// Split *sg* into KernelSubGraph objects by explicit (offset, size) slices.
    ///
    /// Each entry in `slices` is a pair (offset, size), and the kernel covers
    /// features [offset, offset+size). Out-of-bounds or zero-size slices are skipped.
    template<typename T>
    std::vector<KernelSubGraph<T>> splitSubgraphIntoKernels(
        Subgraph<T>& sg,
        const std::vector<std::pair<int, int>>& slices
    ) {
        const int nfeats = sg.getNumFeats();
        if (nfeats <= 0) {
            throw std::invalid_argument("nfeats must be > 0");
        }
        std::vector<KernelSubGraph<T>> kernels;
        kernels.reserve(slices.size());
        for (const auto& s : slices) {
            int start = s.first;
            int sz = s.second;
            if (sz <= 0 || start < 0 || start >= nfeats) continue;
            int end = std::min(start + sz, nfeats);
            if (end <= start) continue;
            kernels.emplace_back(sg, start, end);
        }
        return kernels;
    }

} // namespace opf

#endif // OPF_KERNEL_SUBGRAPH_HPP
