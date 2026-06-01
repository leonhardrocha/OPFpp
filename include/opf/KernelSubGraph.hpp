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

/// Internal overlay for mutable scalar fields.
template<typename T>
struct NodeOverlay {
    bool has_pathval   = false; float   pathval   = 0.f;
    bool has_dens      = false; float   dens      = 0.f;
    bool has_radius    = false; float   radius    = 0.f;
    bool has_label     = false; int     label     = 0;
    bool has_root      = false; int     root      = 0;
    bool has_pred      = false; int     pred      = 0;
    bool has_truelabel = false; int     truelabel = 0;
    bool has_position  = false; int     position  = 0;
    bool has_status    = false; uint8_t status    = 0;
    bool has_relevant  = false; uint8_t relevant  = 0;
    bool has_nplatadj  = false; int     nplatadj  = 0;
};

/// Proxy node returned by KernelSubGraph<T>::getNode().
///
/// Scalar reads check the overlay first, then fall back to the source node.
/// Scalar writes always go to the overlay (lazy copy-on-write).
/// Adjacency accesses use a shared-ownership COW vector.
/// Feature accesses operate on the pre-sliced deep-copy vector.
template<typename T>
class KernelNode {
public:
    KernelNode(
        const Node<T>& source,
        std::shared_ptr<std::vector<T>> feat_slice,
        std::shared_ptr<const std::vector<int>> adj_cow
    )
        : src_(source)
        , feat_(std::move(feat_slice))
        , adj_cow_(std::move(adj_cow))
    {}

    // --- Scalar getters (overlay-first) ---
    float   getPathval()   const { return ov_.has_pathval   ? ov_.pathval   : src_.getPathval(); }
    float   getDens()      const { return ov_.has_dens      ? ov_.dens      : src_.getDens(); }
    float   getRadius()    const { return ov_.has_radius    ? ov_.radius    : src_.getRadius(); }
    int     getLabel()     const { return ov_.has_label     ? ov_.label     : src_.getLabel(); }
    int     getRoot()      const { return ov_.has_root      ? ov_.root      : src_.getRoot(); }
    int     getPred()      const { return ov_.has_pred      ? ov_.pred      : src_.getPred(); }
    int     getTruelabel() const { return ov_.has_truelabel ? ov_.truelabel : src_.getTruelabel(); }
    int     getPosition()  const { return ov_.has_position  ? ov_.position  : src_.getPosition(); }
    uint8_t getStatus()    const { return ov_.has_status    ? ov_.status    : src_.getStatus(); }
    uint8_t getRelevant()  const { return ov_.has_relevant  ? ov_.relevant  : src_.getRelevant(); }
    int     getNplatadj()  const { return ov_.has_nplatadj  ? ov_.nplatadj  : src_.getNplatadj(); }

    // --- Scalar setters (COW overlay) ---
    void setPathval(float v)    { ov_.has_pathval   = true; ov_.pathval   = v; }
    void setDens(float v)       { ov_.has_dens      = true; ov_.dens      = v; }
    void setRadius(float v)     { ov_.has_radius    = true; ov_.radius    = v; }
    void setLabel(int v)        { ov_.has_label     = true; ov_.label     = v; }
    void setRoot(int v)         { ov_.has_root      = true; ov_.root      = v; }
    void setPred(int v)         { ov_.has_pred      = true; ov_.pred      = v; }
    void setTruelabel(int v)    { ov_.has_truelabel = true; ov_.truelabel = v; }
    void setPosition(int v)     { ov_.has_position  = true; ov_.position  = v; }
    void setStatus(uint8_t v)   { ov_.has_status    = true; ov_.status    = v; }
    void setRelevant(uint8_t v) { ov_.has_relevant  = true; ov_.relevant  = v; }
    void setNplatadj(int v)     { ov_.has_nplatadj  = true; ov_.nplatadj  = v; }

    // --- Feature access (deep-sliced copy) ---
    const std::shared_ptr<std::vector<T>>& getFeat() const { return feat_; }
    void setFeat(std::shared_ptr<std::vector<T>> v)  { feat_ = std::move(v); }

    // --- Adjacency (COW) ---
    const std::vector<int>& getAdj() const { return *adj_cow_; }
    std::vector<int>& getAdj() {
        // COW: detach before mutation
        if (adj_cow_.use_count() > 1) {
            adj_cow_ = std::make_shared<const std::vector<int>>(*adj_cow_);
        }
        return const_cast<std::vector<int>&>(*adj_cow_);
    }
    void addToAdj(int idx)  { getAdj().push_back(idx); }
    void clearAdj()         { getAdj().clear(); }

    // Flush dirty overlay back to the source node (optional utility).
    void flush() {
        Node<T>& mut = const_cast<Node<T>&>(src_);
        if (ov_.has_pathval)   mut.setPathval(ov_.pathval);
        if (ov_.has_dens)      mut.setDens(ov_.dens);
        if (ov_.has_radius)    mut.setRadius(ov_.radius);
        if (ov_.has_label)     mut.setLabel(ov_.label);
        if (ov_.has_root)      mut.setRoot(ov_.root);
        if (ov_.has_pred)      mut.setPred(ov_.pred);
        if (ov_.has_truelabel) mut.setTruelabel(ov_.truelabel);
        if (ov_.has_position)  mut.setPosition(ov_.position);
        if (ov_.has_status)    mut.setStatus(ov_.status);
        if (ov_.has_relevant)  mut.setRelevant(ov_.relevant);
        if (ov_.has_nplatadj)  mut.setNplatadj(ov_.nplatadj);
    }

private:
    const Node<T>&  src_;
    std::shared_ptr<std::vector<T>>        feat_;
    std::shared_ptr<const std::vector<int>> adj_cow_;
    NodeOverlay<T>  ov_;
};

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
        // Pre-build per-node proxies
        nodes_.reserve(n);
        for (int i = 0; i < n; ++i) {
            const Node<T>& src_node = source.getNode(i);
            // Deep-copy the feature slice
            const auto& full_feat = *src_node.getFeat();
            auto slice = std::make_shared<std::vector<T>>(
                full_feat.begin() + feat_start_,
                full_feat.begin() + feat_end_
            );
            // Shallow-share the adjacency list
            auto adj_shared = std::make_shared<const std::vector<int>>(src_node.getAdj());
            nodes_.emplace_back(src_node, std::move(slice), std::move(adj_shared));
            // Preserve source node position metadata from parent sg.
            nodes_.back().setPosition(src_node.getPosition());
        }
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
    const KernelNode<T>& getNode(int i) const { return nodes_[i]; }
          KernelNode<T>& getNode(int i)       { return nodes_[i]; }

    // --- Flush all dirty overlays back to source ---
    void flushAll() {
        for (auto& kn : nodes_) kn.flush();
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

    int featStart() const { return feat_start_; }
    int featEnd()   const { return feat_end_;   }

private:
    Subgraph<T>& source_;
    int feat_start_;
    int feat_end_;
    int nfeats_;

    std::vector<KernelNode<T>> nodes_;

    // Local COW scalar fields
    bool has_bestk_   = false; int   local_bestk_   = 0;
    bool has_nlabels_ = false; int   local_nlabels_ = 0;
    bool has_df_      = false; float local_df_      = 0.f;
    bool has_mindens_ = false; float local_mindens_ = 0.f;
    bool has_maxdens_ = false; float local_maxdens_ = 0.f;
    bool has_K_       = false; float local_K_       = 0.f;

    std::vector<int>   kernel_feature_sizes_;
    std::vector<float> kernel_weights_;
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
