#pragma once
#include <concepts>
#include <cstdint>
#include <vector>
#include <cmath>
#include <span>


struct InterleavedLayout {
    template<typename T>
    static inline T get(const std::span<T> data, size_t node_idx, size_t stride, size_t offset, size_t feat_idx) {
        return data[node_idx * stride + offset + feat_idx];
    }
};

struct PlanarLayout {
    template<typename T>
    static inline T get(const std::span<T> data, size_t node_idx, size_t stride, size_t offset, size_t feat_idx) {
        return data[offset + (feat_idx * stride) + node_idx];
    }
};