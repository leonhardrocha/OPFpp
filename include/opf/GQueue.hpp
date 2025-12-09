#pragma once

#include "opf/common.hpp"
#include <vector>
#include <span>

namespace opf {

class GQueue {
public:
    enum class Policy { MIN_VALUE, MAX_VALUE };
    enum class TieBreak { FIFO, LIFO };

    GQueue(int n_buckets, int n_elems, std::span<int> values);

    void Reset();
    bool IsEmpty() const;
    void Insert(int elem);
    int Remove();
    void RemoveElement(int elem);
    void Update(int elem, int new_value);

    void SetTieBreak(TieBreak tb) { m_C.tiebreak = tb; }
    void SetPolicy(Policy p) { m_C.policy = p; }

private:
    struct GQNode {
        int next = 0;
        int prev = 0;
        char color = 0; // WHITE=0, GRAY=1, BLACK=2
    };

    struct DoublyLinkedLists {
        std::vector<GQNode> elem;
        std::span<int> value;
    } m_L;

    struct CircularQueue {
        std::vector<int> first;
        std::vector<int> last;
        int n_buckets = 0;
        int min_value = 0, max_value = 0;
        TieBreak tiebreak = TieBreak::FIFO;
        Policy policy = Policy::MIN_VALUE;
    } m_C;
};

} // namespace opf