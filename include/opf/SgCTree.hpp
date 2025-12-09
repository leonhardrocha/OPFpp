#pragma once

#include "opf/common.hpp"
#include "opf/Subgraph.hpp"
#include <vector>
#include <memory>

namespace opf {

struct SgCTNode {
    int level = 0;
    int comp = 0;
    int dad = NIL;
    std::vector<int> son;
    int size = 0;
};

class SgCTree {
public:
    std::vector<SgCTNode> nodes;
    std::vector<int> cmap;
    int root = NIL;

    explicit SgCTree(const Subgraph& g);

    static std::vector<int> AreaOpen(const Subgraph& g, int thres);
    static std::vector<int> VolumeOpen(const Subgraph& g, int thres);

private:
    static int SgRepresentative(std::vector<int>& cmap, int p);
    static int SgAncestor(const std::vector<int>& dad, std::vector<int>& cmap, int rq);

    void CumSize(int i);
    int AreaLevel(std::vector<int>& level, int i, int thres) const;
    int VolumeLevel(std::vector<int>& level, int i, int thres, int cumvol) const;
};

} // namespace opf