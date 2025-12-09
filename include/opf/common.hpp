#pragma once

#include <string>
#include <stdexcept>
#include <iostream>
#include <random>
#include <vector>

// Constants from the original project
constexpr int NIL = -1;
const std::string MSG1 = "Cannot allocate memory space";

// Modern error handling using exceptions
inline void Error(const std::string& msg, const std::string& func) {
    throw std::runtime_error("Error in " + func + ": " + msg);
}

inline void Warning(const std::string& msg, const std::string& func) {
    std::cerr << "Warning in " << func << ": " << msg << std::endl;
}

// A better random integer generator
inline int RandomInteger(int low, int high) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(low, high);
    return distrib(gen);
}

// Memory allocation functions
int* AllocIntArray(int n);
float* AllocFloatArray(int n);