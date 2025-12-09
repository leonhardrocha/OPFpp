#ifndef SET_HPP
#define SET_HPP

#include <set>
#include <memory>

namespace opf {

template <typename T>
class Set {
public:
    // Default constructor
    Set() = default;

    // Destructor
    ~Set() = default;

    // Copy constructor
    Set(const Set& other) = default;

    // Move constructor
    Set(Set&& other) noexcept = default;

    // Copy assignment operator
    Set& operator=(const Set& other) = default;

    // Move assignment operator
    Set& operator=(Set&& other) noexcept = default;

    void insert(const T& value) {
        data.insert(value);
    }

    void erase(const T& value) {
        data.erase(value);
    }

    bool contains(const T& value) const {
        return data.count(value) > 0;
    }

    size_t size() const {
        return data.size();
    }

    bool empty() const {
        return data.empty();
    }

    void clear() {
        data.clear();
    }

    // Iterators
    typename std::set<T>::iterator begin() {
        return data.begin();
    }

    typename std::set<T>::iterator end() {
        return data.end();
    }

    typename std::set<T>::const_iterator begin() const {
        return data.cbegin();
    }

    typename std::set<T>::const_iterator end() const {
        return data.cend();
    }

private:
    std::set<T> data;
};

// A concrete class for integers
using IntSet = Set<int>;

// For compatibility with the old C-style API, we can provide some helper functions.
// Note: This will require significant refactoring of the calling code.

// A function to mimic the old InsertSet(&set, elem)
inline void InsertSet(std::unique_ptr<IntSet>& set, int elem) {
    if (!set) {
        set = std::make_unique<IntSet>();
    }
    set->insert(elem);
}

// A function to mimic the old RemoveSet(&set)
inline int RemoveSet(std::unique_ptr<IntSet>& set) {
    if (!set || set->empty()) {
        return -1; // Or some other indicator for empty set, NIL from the original code
    }
    int elem = *set->begin();
    set->erase(elem);
    return elem;
}

} // namespace opf

// For backward compatibility, we can have a typedef in the global namespace.
// This is not ideal but can help with the transition.
using OpfSet = opf::Set<int>;


#endif // SET_HPP