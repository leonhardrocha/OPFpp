#ifndef OPF_STRIDED_SUBGRAPH_HPP
#define OPF_STRIDED_SUBGRAPH_HPP

#include <opf/KernelLayout.hpp>
#include <opf/Subgraph.hpp>

#include <memory>
#include <stdexcept>
#include <vector>

namespace opf {

template<typename T>
class StridedNodeView {
public:
    StridedNodeView(const Node<T>& source, int feat_offset, int feat_size)
        : source_(source), feat_offset_(feat_offset), feat_size_(feat_size) {}

    int getPosition() const { return source_.getPosition(); }
    int getLabel() const { return source_.getLabel(); }
    int getRoot() const { return source_.getRoot(); }
    int getPred() const { return source_.getPred(); }
    int getTruelabel() const { return source_.getTruelabel(); }
    float getPathval() const { return source_.getPathval(); }
    float getDens() const { return source_.getDens(); }
    float getRadius() const { return source_.getRadius(); }
    uint8_t getStatus() const { return source_.getStatus(); }
    uint8_t getRelevant() const { return source_.getRelevant(); }
    int getNplatadj() const { return source_.getNplatadj(); }
    const std::vector<int>& getAdj() const { return source_.getAdj(); }

    std::vector<T> getFeatSlice() const {
        const auto& feat = *source_.getFeat();
        if (feat_offset_ < 0 || feat_size_ < 0 || feat_offset_ + feat_size_ > static_cast<int>(feat.size())) {
            throw std::out_of_range("StridedNodeView feature slice out of bounds");
        }
        return std::vector<T>(feat.begin() + feat_offset_, feat.begin() + feat_offset_ + feat_size_);
    }

private:
    const Node<T>& source_;
    int feat_offset_ = 0;
    int feat_size_ = 0;
};

template<typename T>
class StridedSubgraph {
public:
    StridedSubgraph(Subgraph<T>& parent, KernelLayout layout)
        : parent_(parent), layout_(std::move(layout)) {
        layout_.validateAgainstFeatureCount(parent_.getNumFeats());
    }

    Subgraph<T>& parent() { return parent_; }
    const Subgraph<T>& parent() const { return parent_; }

    const KernelLayout& layout() const { return layout_; }

    int getNumNodes() const { return parent_.getNumNodes(); }
    int getNumFeats() const { return parent_.getNumFeats(); }
    int getNumKernels() const { return layout_.numKernels(); }

    bool hasKernel(int kernel_id) const { return layout_.hasKernel(kernel_id); }
    int getKernelOffset(int kernel_id) const { return layout_.offset(kernel_id); }
    int getKernelSize(int kernel_id) const { return layout_.size(kernel_id); }
    int getKernelEnd(int kernel_id) const { return layout_.end(kernel_id); }

    StridedNodeView<T> getNode(int kernel_id, int node_idx) const {
        if (node_idx < 0 || node_idx >= parent_.getNumNodes()) {
            throw std::out_of_range("node_idx out of range in StridedSubgraph");
        }
        return StridedNodeView<T>(
            parent_.getNode(node_idx),
            layout_.offset(kernel_id),
            layout_.size(kernel_id)
        );
    }

    std::vector<float> computeLogDensityProbabilities(int kernel_id, float epsilon = 1e-12f) const {
        if (!layout_.hasKernel(kernel_id)) {
            throw std::out_of_range("Unknown kernel_id in StridedSubgraph");
        }
        std::vector<float> probs(parent_.getNumNodes(), 0.0f);
        for (int i = 0; i < parent_.getNumNodes(); ++i) {
            float dens = parent_.getNode(i).getDens();
            if (dens > epsilon) {
                float p = std::log(dens);
                probs[i] = std::isfinite(p) ? p : 0.0f;
            }
        }
        return probs;
    }

    Subgraph<T> makeKernelCopy(int kernel_id) const {
        const int feat_offset = layout_.offset(kernel_id);
        const int feat_size = layout_.size(kernel_id);
        Subgraph<T> out(parent_.getNumNodes());
        out.setNumFeats(feat_size);
        out.setNumLabels(parent_.getNumLabels());
        out.setBestK(parent_.getBestK());
        out.setDf(parent_.getDf());
        out.setK(parent_.getK());
        out.setMinDens(parent_.getMinDens());
        out.setMaxDens(parent_.getMaxDens());

        for (int i = 0; i < parent_.getNumNodes(); ++i) {
            const Node<T>& src = parent_.getNode(i);
            Node<T>& dst = out.getNode(i);
            dst.setPosition(src.getPosition());
            dst.setTruelabel(src.getTruelabel());
            dst.setLabel(src.getLabel());
            dst.setRoot(src.getRoot());
            dst.setPred(src.getPred());
            dst.setPathval(src.getPathval());
            dst.setDens(src.getDens());
            dst.setRadius(src.getRadius());
            dst.setStatus(src.getStatus());
            dst.setRelevant(src.getRelevant());
            dst.setNplatadj(src.getNplatadj());
            dst.getAdj() = src.getAdj();

            const auto& full_feat = *src.getFeat();
            auto slice = std::make_shared<std::vector<T>>(
                full_feat.begin() + feat_offset,
                full_feat.begin() + feat_offset + feat_size
            );
            dst.setFeat(slice);
        }

        return out;
    }

private:
    Subgraph<T>& parent_;
    KernelLayout layout_;
};

} // namespace opf

#endif // OPF_STRIDED_SUBGRAPH_HPP
