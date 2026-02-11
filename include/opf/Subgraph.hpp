#pragma once

#include "opf/common.hpp"
#include <vector>
#include <span>
#include <string>
#include <list>
#include <memory>

namespace opf {

struct SNode {
    std::span<float> feat;
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
        //std::vector<int> deep_copied_vector_cpp23(view_span);

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
};

class Subgraph {
public:
    std::vector<SNode> nodes;
    std::vector<int> ordered_list_of_nodes;
    size_t nlabels = 0;
    size_t nfeats = 0;
    float df = 0.0f;
    float K = 0.0f;
    float mindens = 0.0f;
    float maxdens = 0.0f;
    int bestk = 0;

    // Construction and I/O
    Subgraph(size_t n_nodes = 0, size_t n_feats = 0, size_t n_labels = 0);
    static std::unique_ptr<Subgraph> Read(const std::string& filename);
    static std::unique_ptr<Subgraph> ReadFromText(const std::string& filename);
    void Write(const std::string& filename) const;
    void WriteAsText(const std::string& filename) const;

    // Data Manipulation
    static std::unique_ptr<Subgraph> Merge(const Subgraph& sg1, const Subgraph& sg2);
    std::unique_ptr<Subgraph> Split(float split_perc);
    void NormalizeFeatures();

    // Core OPF Algorithms
    void Train();
    void Classify(Subgraph& test_sg);

    enum class UnsupervisedFilterType { None, Height, Area, Volume };
    void Cluster(int k_max, UnsupervisedFilterType filter_type = UnsupervisedFilterType::None, float filter_param = 0.0f);

    // Distance function (can be set to different implementations)
    using DistanceFunction = float (*)(const std::vector<float>&, const std::vector<float>&);
    static DistanceFunction Distance;

private:
    // Algorithm implementations
    void MstPrototypes();
    void CreateArcs(int knn);
    void otherroyArcs();
    void ComputePDF();
    void RunClusteringStep();
    float CalculateNormalizedCut();

public:
    // Distance function implementations
    static float EuclideanDistance(const std::vector<float>& f1, const std::vector<float>& f2);
    static float EuclideanDistanceLog(const std::vector<float>& f1, const std::vector<float>& f2);
};

} // namespace opf