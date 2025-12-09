#include "opf/SgCTree.hpp"
#include "opf/GQueue.hpp"
#include <limits>
#include "opf/Subgraph.hpp" // Required for Subgraph class definition
#include <numeric>
#include <algorithm>

namespace opf {

SgCTree::SgCTree(const Subgraph& g) {
    const size_t n = g.nodes.size();
    if (n == 0) return;

    std::vector<int> dad(n);
    std::vector<int> level(n);
    std::vector<int> val(n);
    int Imax = std::numeric_limits<int>::min();

    for (size_t p = 0; p < n; ++p) {
        val[p] = static_cast<int>(g.nodes[p].dens);
        if (val[p] > Imax) {
            Imax = val[p];
        }
    }

    this->cmap.resize(n);
    std::vector<int> size(n);
    GQueue Q(Imax + 1, n, level);
    Q.SetTieBreak(GQueue::TieBreak::LIFO);

    for (size_t p = 0; p < n; ++p) {
        dad[p] = NIL;
        this->cmap[p] = p;
        level[p] = Imax - val[p];
        size[p] = 1;
        Q.Insert(p);
    }

    while (!Q.IsEmpty()) {
        int p = Q.Remove();
        int rp = SgRepresentative(this->cmap, p);

        for (int q : g.nodes[p].adj) {
            if (val[p] == val[q]) {
                if (Q.m_L.elem[q].color == 1 /*GRAY*/) { // Accessing internal for perf
                    this->cmap[q] = rp;
                    if (p == rp) size[rp] += 1;
                    Q.Update(q, level[p]);
                }
            } else if (val[p] < val[q]) {
                int rq = SgRepresentative(this->cmap, q);
                int r = SgAncestor(dad, this->cmap, rq);
                if (r == NIL) {
                    dad[rq] = rp;
                } else {
                    if (val[r] == val[rp]) {
                        if (r != rp) {
                            if (size[rp] <= size[r]) {
                                this->cmap[rp] = r;
                                size[r] += size[rp];
                                rp = r;
                            } else {
                                this->cmap[r] = rp;
                                size[rp] += size[r];
                            }
                        }
                    } else { // val[r] > val[rp]
                        dad[r] = rp;
                    }
                }
            }
        }
    }

    size_t numnodes = 0;
    for (size_t p = 0; p < n; ++p) {
        this->cmap[p] = SgRepresentative(this->cmap, p);
        if (this->cmap[p] == p) {
            numnodes++;
        }
    }

    this->nodes.resize(numnodes);
    std::vector<int> tmp(n, NIL);

    int i = 0;
    for (size_t p = 0; p < n; ++p) {
        if (this->cmap[p] == p) {
            this->nodes[i].level = val[p];
            this->nodes[i].comp = p;
            tmp[p] = i;
            i++;
        }
    }

    for (size_t p = 0; p < n; ++p) {
        if (tmp[p] == NIL) {
            tmp[p] = tmp[this->cmap[p]];
        }
    }
    this->cmap = tmp;

    for (size_t i_node = 0; i_node < this->nodes.size(); ++i_node) {
        int comp_dad = dad[this->nodes[i_node].comp];
        if (comp_dad != NIL) {
            this->nodes[i_node].dad = this->cmap[comp_dad];
        } else {
            this->root = i_node;
        }
    }

    for (size_t i_node = 0; i_node < this->nodes.size(); ++i_node) {
        int p_dad = this->nodes[i_node].dad;
        if (p_dad != NIL) {
            this->nodes[p_dad].son.push_back(i_node);
        }
    }

    for (size_t p = 0; p < n; ++p) {
        this->nodes[this->cmap[p]].size++;
    }
}

int SgCTree::SgRepresentative(std::vector<int>& cmap, int p) {
    if (cmap[p] == p) {
        return p;
    }
    return cmap[p] = SgRepresentative(cmap, cmap[p]);
}

int SgCTree::SgAncestor(const std::vector<int>& dad, std::vector<int>& cmap, int rq) {
    int r, ro;
    ro = r = dad[rq];
    while (r != NIL) {
        ro = r = SgRepresentative(cmap, r);
        r = dad[r];
    }
    return ro;
}

void SgCTree::CumSize(int i) {
    for (int s : this->nodes[i].son) {
        CumSize(s);
        this->nodes[i].size += this->nodes[s].size;
    }
}

int SgCTree::AreaLevel(std::vector<int>& level, int i, int thres) const {
    if (i == NIL) return 0;

    if ((this->nodes[i].size > thres) || (i == this->root)) {
        return this->nodes[i].level;
    }
    return level[i] = AreaLevel(level, this->nodes[i].dad, thres);
}

std::vector<int> SgCTree::AreaOpen(const Subgraph& g, int thres) {
    SgCTree ctree(g);
    if (ctree.root != NIL) {
        ctree.CumSize(ctree.root);
    }

    std::vector<int> level(ctree.nodes.size());
    for (size_t i = 0; i < ctree.nodes.size(); ++i) {
        level[i] = ctree.nodes[i].level;
    }

    for (size_t i = 0; i < ctree.nodes.size(); ++i) {
        if (ctree.nodes[i].son.empty()) {
            level[i] = ctree.AreaLevel(level, i, thres);
        }
    }

    std::vector<int> fval(g.nodes.size());
    for (size_t p = 0; p < g.nodes.size(); ++p) {
        fval[p] = level[ctree.cmap[p]];
    }
    return fval;
}

int SgCTree::VolumeLevel(std::vector<int>& level, int i, int thres, int cumvol) const {
    if (i == NIL) return 0;

    int vol = cumvol;
    int dad = this->nodes[i].dad;
    if (dad != NIL) {
        vol += std::abs(this->nodes[i].level - this->nodes[dad].level) * this->nodes[i].size;
    }

    if ((vol > thres) || (i == this->root)) {
        return this->nodes[i].level;
    }
    return level[i] = VolumeLevel(level, dad, thres, vol);
}

std::vector<int> SgCTree::VolumeOpen(const Subgraph& g, int thres) {
    SgCTree ctree(g);
    if (ctree.root != NIL) {
        ctree.CumSize(ctree.root);
    }

    std::vector<int> level(ctree.nodes.size());
    for (size_t i = 0; i < ctree.nodes.size(); ++i) {
        level[i] = ctree.nodes[i].level;
    }

    for (size_t i = 0; i < ctree.nodes.size(); ++i) {
        if (ctree.nodes[i].son.empty()) {
            level[i] = ctree.VolumeLevel(level, i, thres, 0);
        }
    }

    std::vector<int> fval(g.nodes.size());
    for (size_t p = 0; p < g.nodes.size(); ++p) {
        fval[p] = level[ctree.cmap[p]];
    }
    return fval;
}

} // namespace opf