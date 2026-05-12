#ifndef OPF_FILE_HPP_
#define OPF_FILE_HPP_

#include <string>
#include "Subgraph.hpp"

namespace opf {
    template<typename T>
    void readSubgraph(const std::string& filename, Subgraph<T>& sg);

    template<typename T>
    void writeSubgraph(const std::string& filename, const Subgraph<T>& sg);
}

#endif // OPF_FILE_HPP_