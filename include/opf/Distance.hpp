#ifndef OPF_DISTANCE_HPP
#define OPF_DISTANCE_HPP

#include <opf/common.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

namespace opf {
    namespace distance {

        // Template distance functions (generic over numeric type T)
        template<typename T>
        T euclDist(const std::vector<T>& f1, const std::vector<T>& f2) {
            T dist = T(0);
            for (size_t i = 0; i < f1.size(); ++i) {
                dist += (f1[i] - f2[i]) * (f1[i] - f2[i]);
            }
            return std::sqrt(dist);
        }

        /// log(1 + euclidean_distance) — mirrors opf_EuclDistLog
        template<typename T>
        T euclDistLog(const std::vector<T>& f1, const std::vector<T>& f2) {
            return std::log(T(1) + euclDist(f1, f2));
        }

        /// Gaussian (RBF) distance: exp(-gamma * squared_euclidean)
        template<typename T>
        T gaussDist(const std::vector<T>& f1, const std::vector<T>& f2, T gamma) {
            T sq = T(0);
            for (size_t i = 0; i < f1.size(); ++i) {
                T d = f1[i] - f2[i];
                sq += d * d;
            }
            return std::exp(-gamma * sq);
        }

        template<typename T>
        T chiSquaredDist(const std::vector<T>& f1, const std::vector<T>& f2) {
            T dist = T(0);
            for (size_t i = 0; i < f1.size(); ++i) {
                if ((f1[i] + f2[i]) > T(0)) {
                    dist += ((f1[i] - f2[i]) * (f1[i] - f2[i])) / (f1[i] + f2[i]);
                }
            }
            return dist;
        }

        template<typename T>
        T manhattanDist(const std::vector<T>& f1, const std::vector<T>& f2) {
            T dist = T(0);
            for (size_t i = 0; i < f1.size(); ++i) {
                dist += std::abs(f1[i] - f2[i]);
            }
            return dist;
        }

        template<typename T>
        T canberraDist(const std::vector<T>& f1, const std::vector<T>& f2) {
            T dist = T(0);
            for (size_t i = 0; i < f1.size(); ++i) {
                if ((f1[i] + f2[i]) > T(0)) {
                    dist += std::abs(f1[i] - f2[i]) / (std::abs(f1[i]) + std::abs(f2[i]));
                }
            }
            return dist;
        }

        template<typename T>
        T squaredChordDist(const std::vector<T>& f1, const std::vector<T>& f2) {
            T dist = T(0);
            for (size_t i = 0; i < f1.size(); ++i) {
                dist += (std::sqrt(f1[i]) - std::sqrt(f2[i])) * (std::sqrt(f1[i]) - std::sqrt(f2[i]));
            }
            return dist;
        }

        template<typename T>
        T squaredChiSquaredDist(const std::vector<T>& f1, const std::vector<T>& f2) {
            T dist = T(0);
            for (size_t i = 0; i < f1.size(); ++i) {
                if ((f1[i] + f2[i]) > T(0)) {
                    dist += ((f1[i] - f2[i]) * (f1[i] - f2[i])) / (f1[i] + f2[i]);
                }
            }
            return dist;
        }

        template<typename T>
        T brayCurtisDist(const std::vector<T>& f1, const std::vector<T>& f2) {
            T num = T(0);
            T den = T(0);
            for (size_t i = 0; i < f1.size(); ++i) {
                num += std::abs(f1[i] - f2[i]);
                den += (f1[i] + f2[i]);
            }
            return (den > T(0)) ? (num / den) : T(0);
        }

    } // namespace distance
} // namespace opf

#endif // OPF_DISTANCE_HPP
