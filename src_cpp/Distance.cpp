#include <opf/Distance.hpp>
#include <numeric>
#include <cmath>
#include <algorithm>

namespace opf {
    namespace distance {

        float euclDist(const std::vector<float>& f1, const std::vector<float>& f2) {
            float dist = 0.0f;
            for (size_t i = 0; i < f1.size(); ++i) {
                dist += (f1[i] - f2[i]) * (f1[i] - f2[i]);
            }
            return std::sqrt(dist);
        }

        float chiSquaredDist(const std::vector<float>& f1, const std::vector<float>& f2) {
            float dist = 0.0f;
            for (size_t i = 0; i < f1.size(); ++i) {
                if ((f1[i] + f2[i]) > 0) {
                    dist += ((f1[i] - f2[i]) * (f1[i] - f2[i])) / (f1[i] + f2[i]);
                }
            }
            return dist;
        }

        float manhattanDist(const std::vector<float>& f1, const std::vector<float>& f2) {
            float dist = 0.0f;
            for (size_t i = 0; i < f1.size(); ++i) {
                dist += std::abs(f1[i] - f2[i]);
            }
            return dist;
        }

        float canberraDist(const std::vector<float>& f1, const std::vector<float>& f2) {
            float dist = 0.0f;
            for (size_t i = 0; i < f1.size(); ++i) {
                if ((f1[i] + f2[i]) > 0) {
                    dist += std::abs(f1[i] - f2[i]) / (std::abs(f1[i]) + std::abs(f2[i]));
                }
            }
            return dist;
        }

        float squaredChordDist(const std::vector<float>& f1, const std::vector<float>& f2) {
            float dist = 0.0f;
            for (size_t i = 0; i < f1.size(); ++i) {
                dist += (std::sqrt(f1[i]) - std::sqrt(f2[i])) * (std::sqrt(f1[i]) - std::sqrt(f2[i]));
            }
            return dist;
        }

        float squaredChiSquaredDist(const std::vector<float>& f1, const std::vector<float>& f2) {
            float dist = 0.0f;
            for (size_t i = 0; i < f1.size(); ++i) {
                if ((f1[i] + f2[i]) > 0) {
                    dist += ((f1[i] - f2[i]) * (f1[i] - f2[i])) / (f1[i] + f2[i]);
                }
            }
            return dist;
        }

        float brayCurtisDist(const std::vector<float>& f1, const std::vector<float>& f2) {
            float num = 0.0f;
            float den = 0.0f;
            for (size_t i = 0; i < f1.size(); ++i) {
                num += std::abs(f1[i] - f2[i]);
                den += (f1[i] + f2[i]);
            }
            return (den > 0) ? (num / den) : 0.0f;
        }

    } // namespace distance
} // namespace opf