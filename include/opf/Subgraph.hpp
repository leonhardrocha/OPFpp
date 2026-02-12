#pragma once

#include "opf/common.hpp"
#include "opf/SNode.hpp"
#include <vector>
#include <string>
#include <list>
#include <memory>

namespace opf
{

    class Subgraph
    {
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
        Subgraph(const Subgraph &other);
        static auto Read(const std::string &filename) -> std::unique_ptr<Subgraph>;
        static auto ReadFromText(const std::string &filename) -> std::unique_ptr<Subgraph>;
        void Write(const std::string &filename) const;
        void WriteAsText(const std::string &filename) const;

        // Data Manipulation
        static std::unique_ptr<Subgraph> Merge(const Subgraph &sg1, const Subgraph &sg2);
        std::unique_ptr<Subgraph> Split(float split_perc);
        void NormalizeFeatures();

        // Core OPF Algorithms
        void Train();
        void Classify(Subgraph &test_sg);

        enum class UnsupervisedFilterType
        {
            None,
            Height,
            Area,
            Volume
        };
        void Cluster(int k_max, UnsupervisedFilterType filter_type = UnsupervisedFilterType::None, float filter_param = 0.0f);

        // Distance function (can be set to different implementations)
        using DistanceFunction = float (*)(const std::vector<float> &, const std::vector<float> &);
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
        static float EuclideanDistance(const std::vector<float> &f1, const std::vector<float> &f2);
        static float EuclideanDistanceLog(const std::vector<float> &f1, const std::vector<float> &f2);
    };

} // namespace opf