#ifndef PRIORITY_HEAP_HPP
#define PRIORITY_HEAP_HPP

#include <vector>
#include <cstdint>

namespace opf
{

    enum class Color : uint8_t
    {
        WHITE,
        GRAY,
        BLACK
    };
    enum class Policy
    {
        MIN_VALUE,
        MAX_VALUE
    };

    template <typename T>
    class PriorityHeap
    {
    private:
        std::vector<T> data;
        std::vector<int> pos;
        std::vector<float> costs;
        std::vector<Color> colors;
        int last = -1;
        Policy policy;
        [[nodiscard]] auto compares(float value, float another) const -> bool;
        void swapNodes(int node, int another);
        void goUp(int index);
        void goUpFast(int index) noexcept;
        void goDown(int index);

    public:
        explicit PriorityHeap(int size, std::vector<float> costs, Policy policy = Policy::MIN_VALUE);
        [[nodiscard]] auto insert(T item, int item_id, float cost) -> bool;
        [[nodiscard]] auto remove() -> T;
        void update(T item, float newCost);
        [[nodiscard]] auto isEmpty() const -> bool;
        void reset();
    };

} // namespace opf

#endif
