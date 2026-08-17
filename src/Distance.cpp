#include <opf/Distance.hpp>

// Distance functions are now fully templated in Distance.hpp (header-only).
// Explicit instantiations below for float and double are compiled into this translation unit
// to ensure these specializations are always available and linked correctly.

namespace opf {
    namespace distance {
        // Explicit template instantiations for float
        template float euclDist<float>(const std::vector<float>&, const std::vector<float>&);
        template float euclDistLog<float>(const std::vector<float>&, const std::vector<float>&);
        template float gaussDist<float>(const std::vector<float>&, const std::vector<float>&, float);
        template float chiSquaredDist<float>(const std::vector<float>&, const std::vector<float>&);
        template float manhattanDist<float>(const std::vector<float>&, const std::vector<float>&);
        template float canberraDist<float>(const std::vector<float>&, const std::vector<float>&);
        template float squaredChordDist<float>(const std::vector<float>&, const std::vector<float>&);
        template float squaredChiSquaredDist<float>(const std::vector<float>&, const std::vector<float>&);
        template float brayCurtisDist<float>(const std::vector<float>&, const std::vector<float>&);

        // Explicit template instantiations for double
        template double euclDist<double>(const std::vector<double>&, const std::vector<double>&);
        template double euclDistLog<double>(const std::vector<double>&, const std::vector<double>&);
        template double gaussDist<double>(const std::vector<double>&, const std::vector<double>&, double);
        template double chiSquaredDist<double>(const std::vector<double>&, const std::vector<double>&);
        template double manhattanDist<double>(const std::vector<double>&, const std::vector<double>&);
        template double canberraDist<double>(const std::vector<double>&, const std::vector<double>&);
        template double squaredChordDist<double>(const std::vector<double>&, const std::vector<double>&);
        template double squaredChiSquaredDist<double>(const std::vector<double>&, const std::vector<double>&);
        template double brayCurtisDist<double>(const std::vector<double>&, const std::vector<double>&);
    } // namespace distance
} // namespace opf