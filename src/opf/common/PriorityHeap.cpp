#include "PriorityHeap.hpp"
#include <algorithm>
#include <stdexcept>

namespace opf
{

    template <typename T>
    PriorityHeap<T>::PriorityHeap(int size, std::vector<float> costs, Policy policy) : policy(policy)
    {
        data.resize(size);
        pos.resize(size);
        colors.resize(size);
        last = -1;
        pos.assign(size, -1);
        costs = std::move(costs);
        colors.assign(size, Color::WHITE);
    }

    template <typename T>
    bool PriorityHeap<T>::compares(float value, float another) const
    {
        return (policy == Policy::MIN_VALUE) ? (value < another) : (value > another);
    }

    template <typename T>
    void PriorityHeap<T>::swapNodes(int node, int another)
    {
        std::swap(data[node], data[another]);
        pos[static_cast<int>(data[node])] = node;
        pos[static_cast<int>(data[another])] = another;
    }

    template <typename T>
    void PriorityHeap<T>::goUp(int index)
    {
        while (index > 0)
        {
            int parent = (index - 1) / 2;
            if (compares(costs[static_cast<int>(data[index])], costs[static_cast<int>(data[parent])]))
            {
                swapNodes(index, parent);
                index = parent;
            }
            else
                break;
        }
    }

    template <typename T>
    void PriorityHeap<T>::goUpFast(int index) noexcept {
        // Obtemos o ponteiro base uma única vez (verifque -O3 flag para garantir que isso seja otimizado)
        float* cost_buf = costs.data();
        T* data_buf = data.data();
        int* pos_buf = pos.data();

        while (index > 0) {
            int parent = (index - 1) / 2;
            // Acesso direto via ponteiro (mesma velocidade do C original)
            if (compares(cost_buf[static_cast<size_t>(data_buf[index])], cost_buf[static_cast<size_t>(data_buf[parent])])) {
                // Swap manual de alta performance
                T temp_d = data_buf[index];
                data_buf[index] = data_buf[parent];
                data_buf[parent] = temp_d;

                pos_buf[static_cast<size_t>(data_buf[index])] = index;
                pos_buf[static_cast<size_t>(data_buf[parent])] = parent;
                
                index = parent;
            } else break;
        }
    }


    template <typename T>
    void PriorityHeap<T>::goDown(int index)
    {
        int target = index;
        while (true)
        {
            int left = (index<<1) + 1; // left child index 2*i + 1
            int right = (index<<1) + 2; // right child index 2*i + 2
            if (left <= last && compares(costs[static_cast<int>(data[left])], costs[static_cast<int>(data[target])]))
                target = left;
            if (right <= last && compares(costs[static_cast<int>(data[right])], costs[static_cast<int>(data[target])]))
                target = right;

            if (target != index)
            {
                swapNodes(index, target);
                index = target;
            }
            else
            {
                break;
            }
        }
    }

    template <typename T>
    auto PriorityHeap<T>::insert(T item, int item_id, float cost) -> bool
    {
        if (last + 1 >= (int)data.size() || colors[item_id] != Color::WHITE)
            return false;

        last++;
        data[last] = item;
        costs[item_id] = cost;
        pos[item_id] = last;
        colors[item_id] = Color::GRAY;
        goUp(last);
        return true;
    }

    template <typename T>
    auto PriorityHeap<T>::remove() -> T
    {
        if (isEmpty())
            throw std::runtime_error("Heap is empty");
        T rootItem = data[0];
        int id = static_cast<int>(rootItem);
        colors[id] = Color::BLACK;
        pos[id] = -1;

        data[0] = data[last--];
        if (!isEmpty())
        {
            pos[static_cast<int>(data[0])] = 0;
            goDown(0);
        }
        return rootItem;
    }

    template <typename T>
    void PriorityHeap<T>::update(T item, float newCost)
    {
        int id = static_cast<int>(item);
        if (colors[id] != Color::GRAY)
            return;
        float oldCost = costs[id];
        costs[id] = newCost;
        if (compares(newCost, oldCost))
            goUp(pos[id]);
        else
            goDown(pos[id]);
    }

    template <typename T>
    auto PriorityHeap<T>::isEmpty() const -> bool { return last < 0; }

    template <typename T>
    void PriorityHeap<T>::reset()
    {
        last = -1;
        std::fill(colors.begin(), colors.end(), Color::WHITE);
        std::fill(pos.begin(), pos.end(), -1);
    }
} // namespace opf
