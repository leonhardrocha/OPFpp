#include "opf/Subgraph.hpp"
#include "opf/PriorityHeap.hpp"
#include "OPFpp.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <climits>

// TODO: These global variables should be removed and passed as parameters or class members.
extern char opf_PrecomputedDistance;
extern float **opf_DistanceValue;

namespace opf {

// Initialize distance function pointer
Subgraph::DistanceFunction Subgraph::Distance = Subgraph::EuclideanDistanceLog;


// Subgraph Implementation
Subgraph::Subgraph(size_t n_nodes, size_t n_feats, size_t n_labels)
    : nodes(n_nodes, SNode(n_feats)),
      ordered_list_of_nodes(n_nodes, 0),
      nlabels(n_labels),
      nfeats(n_feats) {}

Subgraph::Subgraph(const Subgraph& other)
    : nodes(other.nodes),
      ordered_list_of_nodes(other.ordered_list_of_nodes),
      nlabels(other.nlabels),
      nfeats(other.nfeats),
      df(other.df),
      K(other.K),
      mindens(other.mindens),
      maxdens(other.maxdens),
      bestk(other.bestk) {}


std::unique_ptr<Subgraph> Subgraph::Read(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        Error("Unable to open file " + filename, "Subgraph::Read");
    }

    int n_nodes;
    file.read(reinterpret_cast<char*>(&n_nodes), sizeof(int));
    auto subgraph = std::make_unique<Subgraph>(n_nodes);

    file.read(reinterpret_cast<char*>(&subgraph->nlabels), sizeof(size_t));
    file.read(reinterpret_cast<char*>(&subgraph->nfeats), sizeof(size_t));
    file.read(reinterpret_cast<char*>(&subgraph->df), sizeof(float));
    file.read(reinterpret_cast<char*>(&subgraph->K), sizeof(float));
    file.read(reinterpret_cast<char*>(&subgraph->mindens), sizeof(float));
    file.read(reinterpret_cast<char*>(&subgraph->maxdens), sizeof(float));

    for (int i = 0; i < n_nodes; ++i) {
        subgraph->nodes[i].feat.resize(subgraph->nfeats);
        file.read(reinterpret_cast<char*>(&subgraph->nodes[i].position), sizeof(int));
        file.read(reinterpret_cast<char*>(&subgraph->nodes[i].truelabel), sizeof(int));
        file.read(reinterpret_cast<char*>(&subgraph->nodes[i].pred), sizeof(int));
        file.read(reinterpret_cast<char*>(&subgraph->nodes[i].label), sizeof(int));
        file.read(reinterpret_cast<char*>(&subgraph->nodes[i].pathval), sizeof(float));
        file.read(reinterpret_cast<char*>(&subgraph->nodes[i].radius), sizeof(float));
        file.read(reinterpret_cast<char*>(subgraph->nodes[i].feat.data()), subgraph->nfeats * sizeof(float));
    }

    file.read(reinterpret_cast<char*>(subgraph->ordered_list_of_nodes.data()), n_nodes * sizeof(int));

    return subgraph;
}

std::unique_ptr<Subgraph> Subgraph::ReadFromText(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        Error("Unable to open file " + filename, "Subgraph::ReadFromText");
    }

    int n_nodes, n_labels, n_feats;
    file >> n_nodes >> n_labels >> n_feats;

    auto g = std::make_unique<Subgraph>(n_nodes, n_feats, n_labels);

    for (int i = 0; i < n_nodes; ++i) {
        file >> g->nodes[i].position >> g->nodes[i].truelabel;
        g->nodes[i].label = g->nodes[i].truelabel; // Default label is true label
        for (size_t j = 0; j < n_feats; ++j) {
            file >> g->nodes[i].feat[j];
        }
    }
    return g;
}

void Subgraph::Write(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        Error("Unable to open file " + filename, "Subgraph::Write");
    }

    int n_nodes = nodes.size();
    file.write(reinterpret_cast<const char*>(&n_nodes), sizeof(int));
    file.write(reinterpret_cast<const char*>(&nlabels), sizeof(size_t));
    file.write(reinterpret_cast<const char*>(&nfeats), sizeof(size_t));
    file.write(reinterpret_cast<const char*>(&df), sizeof(float));
    file.write(reinterpret_cast<const char*>(&K), sizeof(float));
    file.write(reinterpret_cast<const char*>(&mindens), sizeof(float));
    file.write(reinterpret_cast<const char*>(&maxdens), sizeof(float));

    for (int i = 0; i < n_nodes; ++i) {
        file.write(reinterpret_cast<const char*>(&nodes[i].position), sizeof(int));
        file.write(reinterpret_cast<const char*>(&nodes[i].truelabel), sizeof(int));
        file.write(reinterpret_cast<const char*>(&nodes[i].pred), sizeof(int));
        file.write(reinterpret_cast<const char*>(&nodes[i].label), sizeof(int));
        file.write(reinterpret_cast<const char*>(&nodes[i].pathval), sizeof(float));
        file.write(reinterpret_cast<const char*>(&nodes[i].radius), sizeof(float));
        file.write(reinterpret_cast<const char*>(nodes[i].feat.data()), nfeats * sizeof(float));
    }

    file.write(reinterpret_cast<const char*>(ordered_list_of_nodes.data()), n_nodes * sizeof(int));
}

void Subgraph::WriteAsText(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        Error("Unable to open file " + filename, "Subgraph::WriteAsText");
    }

    file << nodes.size() << " " << nlabels << " " << nfeats << "\n";
    for (const auto& node : nodes) {
        file << node.position << " " << node.label;
        for (float f : node.feat) {
            file << " " << f;
        }
        file << "\n";
    }
}

std::unique_ptr<Subgraph> Subgraph::Merge(const Subgraph& sg1, const Subgraph& sg2) {
    if (sg1.nfeats != sg2.nfeats) {
        Error("Cannot merge subgraphs with different number of features", "Subgraph::Merge");
    }

    size_t total_nodes = sg1.nodes.size() + sg2.nodes.size();
    size_t max_labels = std::max(sg1.nlabels, sg2.nlabels);
    auto merged = std::make_unique<Subgraph>(total_nodes, sg1.nfeats, max_labels);

    std::copy(sg1.nodes.begin(), sg1.nodes.end(), merged->nodes.begin());
    std::copy(sg2.nodes.begin(), sg2.nodes.end(), merged->nodes.begin() + sg1.nodes.size());

    return merged;
}

std::unique_ptr<Subgraph> Subgraph::Split(float split_perc) {
    if (split_perc < 0.0f || split_perc > 1.0f) {
        Error("Split percentage must be between 0.0 and 1.0", "Subgraph::Split");
    }

    std::vector<int> label_counts(nlabels + 1, 0);
    for(const auto& node : nodes) {
        label_counts[node.truelabel]++;
    }

    std::vector<int> sg1_counts(nlabels + 1, 0);
    size_t sg1_total_nodes = 0;
    for(size_t i = 1; i <= nlabels; ++i) {
        sg1_counts[i] = std::max(1, static_cast<int>(split_perc * label_counts[i]));
        sg1_total_nodes += sg1_counts[i];
    }

    auto sg1 = std::make_unique<Subgraph>(sg1_total_nodes, nfeats, nlabels);
    auto sg2 = std::make_unique<Subgraph>(nodes.size() - sg1_total_nodes, nfeats, nlabels);

    std::vector<int> indices(nodes.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), std::mt19937{std::random_device{}()});

    size_t sg1_idx = 0, sg2_idx = 0;
    for(int original_idx : indices) {
        int label = nodes[original_idx].truelabel;
        if (sg1_counts[label] > 0) {
            sg1->nodes[sg1_idx++] = nodes[original_idx];
            sg1_counts[label]--;
        } else {
            sg2->nodes[sg2_idx++] = nodes[original_idx];
        }
    }

    *this = *sg1; // The current subgraph becomes the first part of the split
    return sg2;   // The second part is returned
}

void Subgraph::NormalizeFeatures() {
    if (nodes.empty() || nfeats == 0) return;

    std::vector<float> mean(nfeats, 0.0f);
    std::vector<float> stddev(nfeats, 0.0f);

    for (size_t i = 0; i < nfeats; ++i) {
        for (const auto& node : nodes) {
            mean[i] += node.feat[i];
        }
        mean[i] /= nodes.size();

        for (const auto& node : nodes) {
            stddev[i] += (node.feat[i] - mean[i]) * (node.feat[i] - mean[i]);
        }
        stddev[i] = std::sqrt(stddev[i] / nodes.size());
        if (stddev[i] < 1e-6) stddev[i] = 1.0;
    }

    for (auto& node : nodes) {
        for (size_t i = 0; i < nfeats; ++i) {
            node.feat[i] = (node.feat[i] - mean[i]) / stddev[i];
        }
    }
}

void Subgraph::MstPrototypes() {
    if (nodes.empty()) return;

    std::vector<float> pathval(nodes.size(), std::numeric_limits<float>::max());
    PriorityHeap Q(nodes.size(), pathval);

    for (auto& node : nodes) {
        node.status = 0;
    }

    pathval[0] = 0.0f;
    nodes[0].pred = NIL;
    Q.Insert(0);

    while (!Q.IsEmpty()) {
        int p = Q.Remove();
        nodes[p].pathval = pathval[p];

        int pred = nodes[p].pred;
        if (pred != NIL && nodes[p].truelabel != nodes[pred].truelabel) {
            nodes[p].status = opf_PROTOTYPE;
            nodes[pred].status = opf_PROTOTYPE;
        }

        for (size_t q = 0; q < nodes.size(); ++q) {
            if (p != q) {
                float weight = Distance(nodes[p].feat, nodes[q].feat);
                if (weight < pathval[q]) {
                    nodes[q].pred = p;
                    Q.Update(q, weight);
                }
            }
        }
    }
}

void Subgraph::Train() {
    if (nodes.empty()) return;

    MstPrototypes();

    std::vector<float> pathval(nodes.size(), std::numeric_limits<float>::max());
    PriorityHeap Q(nodes.size(), pathval);

    for (size_t p = 0; p < nodes.size(); ++p) {
        if (nodes[p].status == opf_PROTOTYPE) {
            nodes[p].pred = NIL;
            pathval[p] = 0.0f;
            nodes[p].label = nodes[p].truelabel;
            Q.Insert(p);
        }
    }

    int i = 0;
    while (!Q.IsEmpty()) {
        int p = Q.Remove();
        ordered_list_of_nodes[i++] = p;
        nodes[p].pathval = pathval[p];

        for (size_t q = 0; q < nodes.size(); ++q) {
            if (p != q && pathval[p] < pathval[q]) {
                float weight = Distance(nodes[p].feat, nodes[q].feat);
                float tmp = std::max(pathval[p], weight);
                if (tmp < pathval[q]) {
                    nodes[q].pred = p;
                    nodes[q].label = nodes[p].label;
                    Q.Update(q, tmp);
                }
            }
        }
    }
}

void Subgraph::Classify(Subgraph& test_sg) {
    for (auto& test_node : test_sg.nodes) {
        float min_cost = std::numeric_limits<float>::max();
        int best_label = -1;

        for (int train_idx : ordered_list_of_nodes) {
            const auto& train_node = nodes[train_idx];
            if (min_cost <= train_node.pathval) {
                break; // Optimization from original code
            }
            float weight = Distance(train_node.feat, test_node.feat);
            float cost = std::max(train_node.pathval, weight);
            if (cost < min_cost) {
                min_cost = cost;
                best_label = train_node.label;
            }
        }
        test_node.label = best_label;
    }
}

// Distance Functions
float Subgraph::EuclideanDistance(const std::vector<float>& f1, const std::vector<float>& f2) {
    float dist = 0.0f;
    for (size_t i = 0; i < f1.size(); ++i) {
        dist += (f1[i] - f2[i]) * (f1[i] - f2[i]);
    }
    return dist;
}

float Subgraph::EuclideanDistanceLog(const std::vector<float>& f1, const std::vector<float>& f2) {
    constexpr float opf_MAXARCW = 100000.0f;
    return opf_MAXARCW * std::log(EuclideanDistance(f1, f2) + 1.0f);
}

// --- Unsupervised Clustering Methods ---

void Subgraph::RunClusteringStep() {
    // This is the core of opf_OPFClustering
    std::vector<float> pathval(nodes.size());
    for(size_t i=0; i<nodes.size(); ++i) {
        pathval[i] = nodes[i].pathval;
    }

    PriorityHeap Q(nodes.size(), pathval, PriorityHeap::Policy::MAX_VALUE);

    for (size_t p = 0; p < nodes.size(); ++p) {
        nodes[p].pred = NIL;
        nodes[p].root = p;
        Q.Insert(p);
    }

    int current_label = 0;
    int i = 0;
    while(!Q.IsEmpty()) {
        int p = Q.Remove();
        ordered_list_of_nodes[i++] = p;

        if (nodes[p].pred == NIL) {
            pathval[p] = nodes[p].dens;
            nodes[p].label = current_label++;
        }
        nodes[p].pathval = pathval[p];

        for (int q_idx : nodes[p].adj) {
            if (Q.GetColor(q_idx) != PriorityHeap::BLACK) { // Check if not removed
                float tmp = std::min(pathval[p], nodes[q_idx].dens);
                if (tmp > pathval[q_idx]) {
                    nodes[q_idx].pred = p;
                    nodes[q_idx].root = nodes[p].root;
                    nodes[q_idx].label = nodes[p].label;
                    Q.Update(q_idx, tmp);
                }
            }
        }
    }
    this->nlabels = current_label;
}

float Subgraph::CalculateNormalizedCut() {
    if (nlabels == 0) return 0.0f;

    float ncut = 0.0;
    std::vector<double> acumIC(nlabels, 0.0);
    std::vector<double> acumEC(nlabels, 0.0);

    for (size_t p = 0; p < nodes.size(); ++p) {
        for (int q : nodes[p].adj) {
            float dist = Distance(nodes[p].feat, nodes[q].feat);
            if (dist > 1e-6) {
                if (nodes[p].label == nodes[q].label) {
                    acumIC[nodes[p].label] += 1.0 / dist;
                } else {
                    acumEC[nodes[p].label] += 1.0 / dist;
                }
            }
        }
    }

    for (size_t l = 0; l < nlabels; ++l) {
        if (acumIC[l] + acumEC[l] > 1e-6) {
            ncut += static_cast<float>(acumEC[l] / (acumIC[l] + acumEC[l]));
        }
    }
    return ncut;
}

void Subgraph::Cluster(int k_max, UnsupervisedFilterType filter_type, float filter_param) {
    if (nodes.empty()) return;

    // --- 1. Find best k ---
    float min_cut = std::numeric_limits<float>::max();
    int best_k = k_max;
    const int k_min = 1;

    for (int k = k_min; k <= k_max; ++k) {
        Subgraph temp_graph = *this;
        temp_graph.DestroyArcs();
        temp_graph.CreateArcs(k);
        temp_graph.ComputePDF();
        for(auto& node : temp_graph.nodes) node.pathval = node.dens - 1.0f;
        temp_graph.RunClusteringStep();
        float nc = temp_graph.CalculateNormalizedCut();

        if (nc < min_cut) {
            min_cut = nc;
            best_k = k;
        }
    }
    this->bestk = best_k;

    // --- 2. Setup graph with best k and compute final PDF ---
    DestroyArcs();
    CreateArcs(best_k);
    ComputePDF();

    for(auto& node : nodes) {
        node.pathval = node.dens - 1.0f;
    }

    // --- 3. Apply filtering ---
    if (filter_type != UnsupervisedFilterType::None && filter_param > 0.0f) {
        if (filter_type == UnsupervisedFilterType::Height) {
            float Hmax = 0.0f;
            for(const auto& node : nodes) if (node.dens > Hmax) Hmax = node.dens;
            for(auto& node : nodes) node.pathval = std::max(node.dens - (filter_param * Hmax), 0.0f);
        } else if (filter_type == UnsupervisedFilterType::Area) {
            std::vector<int> area_levels = SgCTree::AreaOpen(*this, static_cast<int>(filter_param * nodes.size()));
            for(size_t i = 0; i < nodes.size(); ++i) nodes[i].pathval = std::max(static_cast<float>(area_levels[i] - 1), 0.0f);
        } else if (filter_type == UnsupervisedFilterType::Volume) {
            double Vmax = 0.0;
            for(const auto& node : nodes) Vmax += node.dens;
            std::vector<int> volume_levels = SgCTree::VolumeOpen(*this, static_cast<int>(filter_param * Vmax / nodes.size()));
            for(size_t i = 0; i < nodes.size(); ++i) nodes[i].pathval = std::max(static_cast<float>(volume_levels[i] - 1), 0.0f);
        }
    }

    // --- 4. Perform final clustering ---
    RunClusteringStep();

    // --- 5. Post-processing to assign final labels ---
    bool has_true_labels = !nodes.empty() && (nodes[0].truelabel != 0);

    if (has_true_labels) {
        size_t max_true_label = 0;
        for(auto& node : nodes) {
            node.label = nodes[node.root].truelabel;
            if (static_cast<size_t>(node.label) > max_true_label) {
                max_true_label = node.label;
            }
        }
        this->nlabels = max_true_label;
    } else {
        for(auto& node : nodes) {
            node.truelabel = node.label + 1; // OPF convention
        }
    }
}

void Subgraph::CreateArcs(int knn) {
    if (nodes.empty() || knn <= 0) return;

    df = 0.0;
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::vector<float> d(knn, std::numeric_limits<float>::max());
        std::vector<int> nn(knn, -1);

        for (size_t j = 0; j < nodes.size(); ++j) {
            if (i == j) continue;

            float current_dist;
            if (!opf_PrecomputedDistance) {
                current_dist = Distance(nodes[i].feat, nodes[j].feat);
            } else {
                current_dist = opf_DistanceValue[nodes[i].position][nodes[j].position];
            }

            if (current_dist < d.back()) {
                d.back() = current_dist;
                nn.back() = j;
                // bubble up to correct position
                for (int k = knn - 1; k > 0; --k) {
                    if (d[k] < d[k-1]) {
                        std::swap(d[k], d[k-1]);
                        std::swap(nn[k], nn[k-1]);
                    } else {
                        break;
                    }
                }
            }
        }

        nodes[i].adj.clear();
        nodes[i].radius = 0.0f;
        for (int l = 0; l < knn; ++l) {
            if (nn[l] != -1) {
                if (d[l] > df) {
                    df = d[l];
                }
                nodes[i].adj.push_back(nn[l]);
            }
        }
        if(!d.empty() && d.back() != std::numeric_limits<float>::max()){
            nodes[i].radius = d.back();
        }
    }

    if (df < 1e-5f) {
        df = 1.0f;
    }
}

void Subgraph::DestroyArcs() {
    for (auto& node : nodes) {
        node.nplatadj = 0;
        node.adj.clear();
    }
}

void Subgraph::ComputePDF() {
    if (nodes.empty()) return;

    K = (2.0f * df / 9.0f);
    if (K < 1e-5f) K = 1e-5f;

    std::vector<float> value(nodes.size(), 0.0f);
    mindens = std::numeric_limits<float>::max();
    maxdens = std::numeric_limits<float>::min();

    for (size_t i = 0; i < nodes.size(); ++i) {
        float dens_val = 0.0f;
        for (int neighbor_idx : nodes[i].adj) {
            float dist;
            if (!opf_PrecomputedDistance) {
                dist = Distance(nodes[i].feat, nodes[neighbor_idx].feat);
            } else {
                dist = opf_DistanceValue[nodes[i].position][nodes[neighbor_idx].position];
            }
            dens_val += std::exp(-dist / K);
        }
        
        if (nodes[i].adj.empty())
            value[i] = 0.0f;
        else
            value[i] = dens_val / (nodes[i].adj.size());

        if (value[i] < mindens) mindens = value[i];
        if (value[i] > maxdens) maxdens = value[i];
    }

    if (std::abs(mindens - maxdens) < 1e-5) {
        for (auto& node : nodes) {
            node.dens = opf_MAXDENS;
            node.pathval = opf_MAXDENS - 1.0f;
        }
    } else {
        for (size_t i = 0; i < nodes.size(); ++i) {
            nodes[i].dens = ((opf_MAXDENS - 1.0f) * (value[i] - mindens) / (maxdens - mindens)) + 1.0f;
            nodes[i].pathval = nodes[i].dens - 1.0f;
        }
    }
}

} // namespace opf