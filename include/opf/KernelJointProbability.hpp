#ifndef OPF_KERNEL_JOINT_PROBABILITY_HPP
#define OPF_KERNEL_JOINT_PROBABILITY_HPP

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <numeric>

namespace opf {

class KernelJointProbabilityAccumulator {
public:
    explicit KernelJointProbabilityAccumulator(
        int nnodes = 0,
        const std::vector<float>& kernel_weights = {}
    )
        : central_sum_(nnodes, 0.0f), nnodes_(nnodes), kernel_weights_(kernel_weights) {}

    int getNumNodes() const { return nnodes_; }

    bool hasKernel(int kernel_id) const {
        return kernel_probs_.find(kernel_id) != kernel_probs_.end();
    }

    void clear() {
        kernel_probs_.clear();
        std::fill(central_sum_.begin(), central_sum_.end(), 0.0f);
    }

    void reset(int nnodes) {
        if (nnodes < 0) {
            throw std::invalid_argument("nnodes must be >= 0");
        }
        nnodes_ = nnodes;
        kernel_probs_.clear();
        central_sum_.assign(nnodes_, 0.0f);
    }

    void setKernelWeights(const std::vector<float>& kernel_weights) {
        kernel_weights_ = kernel_weights;
        recomputeCentralSum();
    }

    const std::vector<float>& getKernelWeights() const {
        return kernel_weights_;
    }

    void setKernelWeight(int kernel_id, float weight) {
        if (kernel_id < 0) {
            throw std::invalid_argument("kernel_id must be >= 0");
        }
        if (kernel_id >= static_cast<int>(kernel_weights_.size())) {
            kernel_weights_.resize(static_cast<size_t>(kernel_id) + 1, 1.0f);
        }
        kernel_weights_[kernel_id] = weight;
        recomputeCentralSum();
    }

    float getKernelWeight(int kernel_id) const {
        if (kernel_id < 0) {
            throw std::invalid_argument("kernel_id must be >= 0");
        }
        if (kernel_id >= static_cast<int>(kernel_weights_.size())) {
            return 1.0f;
        }
        return kernel_weights_[kernel_id];
    }

    void updateKernelProbabilities(int kernel_id, const std::vector<float>& new_probs) {
        validateSize(new_probs);

        auto it = kernel_probs_.find(kernel_id);
        if (it != kernel_probs_.end()) {
            const std::vector<float>& old_probs = it->second;
            const float w = getKernelWeight(kernel_id);
            for (int i = 0; i < nnodes_; ++i) {
                central_sum_[i] -= w * old_probs[i];
            }
        }

        const float w = getKernelWeight(kernel_id);
        for (int i = 0; i < nnodes_; ++i) {
            central_sum_[i] += w * new_probs[i];
        }

        kernel_probs_[kernel_id] = new_probs;
    }

    void removeKernelProbabilities(int kernel_id) {
        auto it = kernel_probs_.find(kernel_id);
        if (it == kernel_probs_.end()) return;

        const std::vector<float>& old_probs = it->second;
        const float w = getKernelWeight(kernel_id);
        for (int i = 0; i < nnodes_; ++i) {
            central_sum_[i] -= w * old_probs[i];
        }
        kernel_probs_.erase(it);
    }

    const std::vector<float>& getCentralJointProbabilities() const {
        return central_sum_;
    }

    float getCentralJointSum() const {
        return std::accumulate(central_sum_.begin(), central_sum_.end(), 0.0f);
    }

private:
    void validateSize(const std::vector<float>& probs) {
        if (nnodes_ == 0) {
            nnodes_ = static_cast<int>(probs.size());
            central_sum_.assign(nnodes_, 0.0f);
        }
        if (static_cast<int>(probs.size()) != nnodes_) {
            throw std::invalid_argument("Probability vector size mismatch with accumulator nnodes");
        }
    }

    void recomputeCentralSum() {
        std::fill(central_sum_.begin(), central_sum_.end(), 0.0f);
        for (const auto& kv : kernel_probs_) {
            const int kernel_id = kv.first;
            const std::vector<float>& probs = kv.second;
            const float w = getKernelWeight(kernel_id);
            for (int i = 0; i < nnodes_; ++i) {
                central_sum_[i] += w * probs[i];
            }
        }
    }

    std::unordered_map<int, std::vector<float>> kernel_probs_;
    std::vector<float> central_sum_;
    int nnodes_ = 0;
    std::vector<float> kernel_weights_;
};

} // namespace opf

#endif // OPF_KERNEL_JOINT_PROBABILITY_HPP
