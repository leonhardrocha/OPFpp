#ifndef OPF_COMPONENT_TREE_HPP
#define OPF_COMPONENT_TREE_HPP

#include <opf/common.hpp>
#include <vector>
#include <queue>
#include <limits>
#include <cmath>
#include <functional>

// Ported from LibOPF/src/util/sgctree.c
// GQueue (bucket priority queue with LIFO tiebreak) is replaced by
// std::priority_queue with lazy-deletion; GQueue color tracking is
// replaced by an explicit `std::vector<uint8_t> color`.

namespace opf {

// ---- SgCTNode -------------------------------------------------------
struct SgCTNode {
    int level   = 0;    ///< density level of this component tree node
    int comp    = 0;    ///< representative pixel (subgraph node index)
    int dad     = NIL;  ///< parent in the max-tree
    std::vector<int> sons;

    int size    = 0;    ///< number of subgraph nodes in this component
};

// ---- SgCTree --------------------------------------------------------
struct SgCTree {
    std::vector<SgCTNode> nodes; ///< tree nodes
    std::vector<int>      cmap; ///< maps each subgraph node → tree node index
    int root     = NIL;
    int numnodes = 0;
};

namespace detail {

// Path-compressed representative finder (union-find)
inline int sgRepresentative(std::vector<int>& cmap, int p) {
    if (cmap[p] == p) return p;
    return cmap[p] = sgRepresentative(cmap, cmap[p]);
}

// Walk up dad[] until NIL, following cmap compression
inline int sgAncestor(std::vector<int>& dad, std::vector<int>& cmap, int rq) {
    int r = dad[rq];
    int ro = r;
    while (r != NIL) {
        ro = r = sgRepresentative(cmap, r);
        r = dad[r];
    }
    return ro;
}

} // namespace detail

// ---- Post-order cumulative size accumulation ------------------------
inline void sgCumSize(SgCTree& ctree, int i) {
    for (int s : ctree.nodes[i].sons) {
        sgCumSize(ctree, s);
        ctree.nodes[i].size += ctree.nodes[s].size;
    }
}

// ---- Level propagation for area opening ----------------------------
inline int sgAreaLevel(SgCTree& ctree, std::vector<int>& level, int i, int thres) {
    if (i == NIL) return 0;
    if (ctree.nodes[i].size > thres || i == ctree.root)
        return ctree.nodes[i].level;
    return level[i] = sgAreaLevel(ctree, level, ctree.nodes[i].dad, thres);
}

// ---- Level propagation for volume opening --------------------------
inline int sgVolumeLevel(SgCTree& ctree, std::vector<int>& level, int i, int thres, int cumvol) {
    if (i == NIL) return 0;
    int vol = cumvol;
    int dad = ctree.nodes[i].dad;
    if (dad != NIL)
        vol = cumvol + std::abs(ctree.nodes[i].level - ctree.nodes[dad].level) * ctree.nodes[i].size;
    if (vol > thres || i == ctree.root)
        return ctree.nodes[i].level;
    return level[i] = sgVolumeLevel(ctree, level, dad, thres, vol);
}

// ---- CreateSgMaxTree ------------------------------------------------
/// Build a max-tree (component tree) from the density values in @p g.
/// Ported from LibOPF's CreateSgMaxTree; GQueue replaced by a lazy
/// std::priority_queue.
template<typename T>
SgCTree createSgMaxTree(const std::vector<int>& adj_list_flat,
                         const std::vector<std::vector<int>>& adj,
                         const std::vector<int>& val, int n) {
    SgCTree ctree;
    ctree.cmap.resize(n);
    std::vector<int> dad(n, NIL);
    std::vector<int> size(n, 1);
    std::vector<uint8_t> color(n, WHITE);  // WHITE=unvisited, GRAY=in queue, BLACK=done

    int Imax = *std::max_element(val.begin(), val.end());

    // level[p] = Imax - val[p]  → smallest level = highest density → processed first
    std::vector<int> level(n);
    for (int p = 0; p < n; ++p) {
        level[p] = Imax - val[p];
        ctree.cmap[p] = p;
        color[p] = GRAY;
    }

    // Lazy min-heap: (level, node)
    using Elem = std::pair<int, int>;
    std::priority_queue<Elem, std::vector<Elem>, std::greater<Elem>> Q;
    for (int p = 0; p < n; ++p)
        Q.push({level[p], p});

    while (!Q.empty()) {
        auto [lp, p] = Q.top(); Q.pop();
        if (color[p] == BLACK) continue;  // stale entry
        color[p] = BLACK;

        int rp = detail::sgRepresentative(ctree.cmap, p);

        for (int q : adj[p]) {
            if (val[p] == val[q]) {
                if (color[q] == GRAY) {
                    ctree.cmap[q] = rp;
                    if (p == rp) size[rp] += 1;
                    // update: re-insert with level of p
                    level[q] = level[p];
                    color[q] = GRAY;  // keep GRAY, re-push
                    Q.push({level[p], q});
                }
            } else if (val[p] < val[q]) {
                int rq = detail::sgRepresentative(ctree.cmap, q);
                int r  = detail::sgAncestor(dad, ctree.cmap, rq);
                if (r == NIL) {
                    dad[rq] = rp;
                } else {
                    if (val[r] == val[rp]) {
                        if (r != rp) {
                            if (size[rp] <= size[r]) {
                                ctree.cmap[rp] = r;
                                size[r] += size[rp];
                                rp = r;
                            } else {
                                ctree.cmap[r] = rp;
                                size[rp] += size[r];
                            }
                        }
                    } else {
                        dad[r] = rp;
                    }
                }
            }
        }
    }

    // Compress cmap and count tree nodes
    ctree.numnodes = 0;
    for (int p = 0; p < n; ++p) {
        ctree.cmap[p] = detail::sgRepresentative(ctree.cmap, p);
        if (ctree.cmap[p] == p) ctree.numnodes++;
    }

    // Create tree node array
    ctree.nodes.resize(ctree.numnodes);
    std::vector<int> tmp(n, NIL);
    int i = 0;
    for (int p = 0; p < n; ++p) {
        if (ctree.cmap[p] == p) {
            ctree.nodes[i].level = val[p];
            ctree.nodes[i].comp  = p;
            ctree.nodes[i].dad   = NIL;
            ctree.nodes[i].size  = 0;
            tmp[p] = i;
            i++;
        }
    }

    // Fill tmp for non-representative nodes
    for (int p = 0; p < n; ++p)
        if (tmp[p] == NIL) tmp[p] = tmp[ctree.cmap[p]];
    for (int p = 0; p < n; ++p)
        ctree.cmap[p] = tmp[p];

    // Copy dad info and find root
    for (int ii = 0; ii < ctree.numnodes; ++ii) {
        int comp = ctree.nodes[ii].comp;
        if (dad[comp] != NIL) {
            ctree.nodes[ii].dad = ctree.cmap[dad[comp]];
        } else {
            ctree.nodes[ii].dad = NIL;
            ctree.root = ii;
        }
    }

    // Build sons lists
    for (int ii = 0; ii < ctree.numnodes; ++ii) {
        int parent = ctree.nodes[ii].dad;
        if (parent != NIL)
            ctree.nodes[parent].sons.push_back(ii);
    }

    // Compute per-node pixel counts
    for (int p = 0; p < n; ++p)
        ctree.nodes[ctree.cmap[p]].size++;

    return ctree;
}

// ---- SgAreaOpen ----------------------------------------------------
/// Area opening on a subgraph: returns filtered density level per node.
template<typename T>
std::vector<int> sgAreaOpen(const std::vector<std::vector<int>>& adj,
                             const std::vector<int>& densInt, int n, int thres) {
    SgCTree ctree = createSgMaxTree<T>(
        std::vector<int>(), adj, densInt, n);

    sgCumSize(ctree, ctree.root);

    std::vector<int> level(ctree.numnodes);
    for (int i = 0; i < ctree.numnodes; ++i)
        level[i] = ctree.nodes[i].level;

    for (int i = 0; i < ctree.numnodes; ++i)
        if (ctree.nodes[i].sons.empty())
            level[i] = sgAreaLevel(ctree, level, i, thres);

    std::vector<int> fval(n);
    for (int p = 0; p < n; ++p)
        fval[p] = level[ctree.cmap[p]];
    return fval;
}

// ---- SgVolumeOpen --------------------------------------------------
/// Volume opening on a subgraph: returns filtered density level per node.
template<typename T>
std::vector<int> sgVolumeOpen(const std::vector<std::vector<int>>& adj,
                               const std::vector<int>& densInt, int n, int thres) {
    SgCTree ctree = createSgMaxTree<T>(
        std::vector<int>(), adj, densInt, n);

    sgCumSize(ctree, ctree.root);

    std::vector<int> level(ctree.numnodes);
    for (int i = 0; i < ctree.numnodes; ++i)
        level[i] = ctree.nodes[i].level;

    for (int i = 0; i < ctree.numnodes; ++i)
        if (ctree.nodes[i].sons.empty())
            level[i] = sgVolumeLevel(ctree, level, i, thres, 0);

    std::vector<int> fval(n);
    for (int p = 0; p < n; ++p)
        fval[p] = level[ctree.cmap[p]];
    return fval;
}

} // namespace opf

#endif // OPF_COMPONENT_TREE_HPP
