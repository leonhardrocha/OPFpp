#ifndef OPF_DISTANCE_HPP
#define OPF_DISTANCE_HPP

#include <opf/common.hpp>

namespace opf {
    namespace distance {

        float euclDist(const std::vector<float>& f1, const std::vector<float>& f2);
        float chiSquaredDist(const std::vector<float>& f1, const std::vector<float>& f2);
        float manhattanDist(const std::vector<float>& f1, const std::vector<float>& f2);
        float canberraDist(const std::vector<float>& f1, const std::vector<float>& f2);
        float squaredChordDist(const std::vector<float>& f1, const std::vector<float>& f2);
        float squaredChiSquaredDist(const std::vector<float>& f1, const std::vector<float>& f2);
        float brayCurtisDist(const std::vector<float>& f1, const std::vector<float>& f2);

    } // namespace distance
} // namespace opf

#endif // OPF_DISTANCE_HPP
