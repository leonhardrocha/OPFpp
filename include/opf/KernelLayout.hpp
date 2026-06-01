#ifndef OPF_KERNEL_LAYOUT_HPP
#define OPF_KERNEL_LAYOUT_HPP

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace opf {

struct KernelSlice {
    int kernel_id = -1;
    int offset = 0;
    int size = 0;

    int end() const { return offset + size; }
};

class KernelLayout {
public:
    KernelLayout() = default;

    explicit KernelLayout(const std::vector<KernelSlice>& slices)
        : slices_(slices) {
        validateAndIndex();
    }

    explicit KernelLayout(const std::vector<std::pair<int, int>>& offset_size_slices) {
        slices_.reserve(offset_size_slices.size());
        for (size_t idx = 0; idx < offset_size_slices.size(); ++idx) {
            slices_.push_back(KernelSlice{
                static_cast<int>(idx),
                offset_size_slices[idx].first,
                offset_size_slices[idx].second,
            });
        }
        validateAndIndex();
    }

    int numKernels() const { return static_cast<int>(slices_.size()); }

    bool empty() const { return slices_.empty(); }

    bool hasKernel(int kernel_id) const {
        return slice_index_by_id_.find(kernel_id) != slice_index_by_id_.end();
    }

    const KernelSlice& slice(int kernel_id) const {
        auto it = slice_index_by_id_.find(kernel_id);
        if (it == slice_index_by_id_.end()) {
            throw std::out_of_range("Unknown kernel_id in KernelLayout");
        }
        return slices_[it->second];
    }

    int offset(int kernel_id) const { return slice(kernel_id).offset; }
    int size(int kernel_id) const { return slice(kernel_id).size; }
    int end(int kernel_id) const { return slice(kernel_id).end(); }

    const std::vector<KernelSlice>& slices() const { return slices_; }

    std::vector<int> orderedKernelIds() const {
        std::vector<int> ids;
        ids.reserve(slices_.size());
        for (const auto& s : slices_) {
            ids.push_back(s.kernel_id);
        }
        return ids;
    }

    void validateAgainstFeatureCount(int nfeats) const {
        if (nfeats < 0) {
            throw std::invalid_argument("nfeats must be >= 0");
        }
        for (const auto& s : slices_) {
            if (s.offset < 0 || s.size <= 0 || s.end() > nfeats) {
                throw std::invalid_argument("KernelLayout slice out of feature bounds");
            }
        }
    }

private:
    void validateAndIndex() {
        slice_index_by_id_.clear();
        for (size_t idx = 0; idx < slices_.size(); ++idx) {
            const auto& s = slices_[idx];
            if (s.kernel_id < 0) {
                throw std::invalid_argument("kernel_id must be >= 0");
            }
            if (s.offset < 0) {
                throw std::invalid_argument("kernel offset must be >= 0");
            }
            if (s.size <= 0) {
                throw std::invalid_argument("kernel size must be > 0");
            }
            if (!slice_index_by_id_.emplace(s.kernel_id, idx).second) {
                throw std::invalid_argument("duplicate kernel_id in KernelLayout");
            }
        }
    }

    std::vector<KernelSlice> slices_;
    std::unordered_map<int, size_t> slice_index_by_id_;
};

} // namespace opf

#endif // OPF_KERNEL_LAYOUT_HPP
