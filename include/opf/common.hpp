#pragma once

#include <string>
#include <stdexcept>
#include <iostream>
#include <random>
#include <vector>
#include <climits>




// Constants from the original project
constexpr int NIL = -1;
/* Error messages */
const std::string MSG1 = "Cannot allocate memory space";
const std::string MSG2 = "Cannot open file";
const std::string MSG3 = "Invalid option";

/* Common definitions */
#define PI          3.1415926536
#define INTERIOR    0
#define EXTERIOR    1
#define BOTH        2
#define WHITE       0
#define GRAY        1
#define BLACK       2
#define NIL        -1
#define INCREASING  1
#define DECREASING  0
#define Epsilon     1E-05

/* Common operations */
#ifndef MAX
#define MAX(x,y) (((x) > (y))?(x):(y))
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y))?(x):(y))
#endif


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