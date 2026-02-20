#ifndef OPF_COMMON_HPP_
#define OPF_COMMON_HPP_

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <limits>
#include <cfloat>
#include <chrono>
#include <algorithm>
#include <random>
#include <iostream>
#include <stdexcept>

namespace opf {

// Error messages
constexpr const char* const MSG1 = "Cannot allocate memory space";
constexpr const char* const MSG2 = "Cannot open file";
constexpr const char* const MSG3 = "Invalid option";

// Common data types
using timer = std::chrono::high_resolution_clock;

// Common definitions
constexpr double PI = 3.1415926536;
constexpr int INTERIOR = 0;
constexpr int EXTERIOR = 1;
constexpr int BOTH = 2;
constexpr int WHITE = 0;
constexpr int GRAY = 1;
constexpr int BLACK = 2;
constexpr int NIL = -1;
constexpr bool INCREASING = true;
constexpr bool DECREASING = false;
constexpr double Epsilon = 1E-05;

// Common operations are replaced by std::min and std::max

// Error and Warning functions
inline void Error(const char* msg, const char* func) {
    fprintf(stderr, "Error in %s: %s\n", func, msg);
    throw std::runtime_error(msg);
}

inline void Warning(const char* msg, const char* func) {
    fprintf(stderr, "Warning in %s: %s\n", func, msg);
}

// std::swap replaces Change

// Modern C++ random number generation
inline int RandomInteger(int low, int high) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(low, high);
    return distrib(gen);
}

} // namespace opf

#endif // OPF_COMMON_HPP_
