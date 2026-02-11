#pragma once

#include "common.hpp"
#include <vector>
#include <span>

class RealHeap {
public:
    enum class Policy { MIN_VALUE, MAX_VALUE };
    enum Color { WHITE, GRAY, BLACK };

    RealHeap(int n, std::span<float> cost, Policy policy = Policy::MIN_VALUE);

    void SetPolicy(Policy policy);
    bool IsFull() const;
    bool IsEmpty() const;
    bool Insert(int pixel);
    int Remove(); // Throws if empty
    void Update(int p, float value);
    void Reset();
    char GetColor(int index) const { return m_color.at(index); }

private:
    void GoUp(int i);
    void GoDown(int i);

    int m_n;
    std::span<float> m_cost;
    std::vector<char> m_color;
    std::vector<int> m_pixel;
    std::vector<int> m_pos;
    int m_last;
    Policy m_policy;


};