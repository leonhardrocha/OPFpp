#pragma once
#include <cstdint>
#include <concepts>
#include <vector>
#include <cmath>

// Restrição opcional: garante que o buffer lide apenas com tipos flutuantes ou numéricos
template<typename T>
concept NumericBuffer = std::is_arithmetic_v<T>;

template<typename T>
concept ObjectBuffer = std::is_class_v<T> || std::is_struct_v<T>;
