#include "opf/common.hpp"
#include <cstdlib> // For rand()
#include <new>     // For std::bad_alloc

namespace opf {

    int* AllocIntArray(int n) {
        try {
            return new int[n]();
        } catch (const std::bad_alloc& e) {
            Error(MSG1, "AllocIntArray");
        }
        return nullptr; // Should not be reached
    }

    float* AllocFloatArray(int n) {
        try {
            return new float[n]();
        } catch (const std::bad_alloc& e) {
            Error(MSG1, "AllocFloatArray");
        }
        return nullptr; // Should not be reached
    }

    // This is a local utility function, not exposed in the header.
    // A better implementation would be std::swap.
    void Change(int& a, int& b) {
        const int c = a;
        a = b;
        b = c;
    }

} // namespace opf