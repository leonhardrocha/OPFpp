#pragma once
#include <nlohmann/json.hpp>
#include "opf/common.hpp"
#include <vector>
#include <list>
#include <cstddef>

using namespace opf;

struct SNode {
    std::vector<float> feat;
    std::list<int> adj;
    int position = 0;
    int truelabel = 0;
    int label = 0;
    int pred = NIL;
    int root = NIL;
    float pathval = 0.0f;
    float radius = 0.0f;
    float dens = 0.0f;
    int status = 0;
    int relevant = 0;
    int nplatadj = 0;

    SNode() = default;
    SNode(const SNode& other)
    {
        feat = std::vector<float>(other.feat.begin(), other.feat.end());
        adj = std::list<int>(other.adj.begin(), other.adj.end());

        pathval = other.pathval;
        dens = other.dens;
        label  = other.label;
        root = other.root;
        pred  = other.pred;
        truelabel = other.truelabel;
        position = other.position;
        status = other.status;
        relevant = other.relevant;
        radius = other.radius;
        nplatadj = other.nplatadj;
    };
    explicit SNode(size_t n_feats);

    void from_json(const json &j);
    auto to_json() -> std::unique_ptr<json>
};
