#include "opf/SNode.hpp"

using namespace opf;
using json = nlohmann::json;
const int NIL = -1;

struct SNode {

    // Object -> JSON
    auto to_json() -> std::unique_ptr<json>    
    {
        // Convertfeat (vector<float>) to binary -> base64
        auto span_feat = std::span(reinterpret_cast<const std::uint8_t*>(feat.data()), 
                            feat.size() * sizeof(float));
        std::string feat_b64 = base64::encode(span_feat);

        // Converte adj (list<int>) para um buffer temporário -> base64
        std::vector<int> adj_vec(adj.begin(), adj.end());
        auto span_adj = std::span(reinterpret_cast<const std::uint8_t*>(adj_vec.data()), 
                            adj_vec.size() * sizeof(int));
        std::string adj_b64 = base64::encode(span_adj);

        j = json{
            {"feat", feat_b64}, {"adj", adj_b64},
            {"position", position}, {"truelabel", truelabel},
            {"label", label}, {"pred", pred}, {"root", root},
            {"pathval", pathval}, {"radius", radius}, {"dens", dens},
            {"status", status}, {"relevant", relevant}, {"nplatadj", nplatadj}
        };

        return j;
    }

    // JSON -> Object
    void from_json(const json& j)
    {
        // Decode feat
        std::string feat_bin = base64_decode(j.at("feat").get<std::string>());
        n.feat.resize(feat_bin.size() / sizeof(float));
        std::memcpy(feat.data(), feat_bin.data(), feat_bin.size());

        // Decode adj
        std::string adj_bin = base64::decode(j.at("adj").get<std::string>());
        size_t adj_count = adj_bin.size() / sizeof(int);
        std::vector<int> temp_adj(adj_count);
        std::memcpy(temp_adj.data(), adj_bin.data(), adj_bin.size());
        adj.assign(temp_adj.begin(), temp_adj.end());

        // other fields
        j.at("position").get_to(position);
        j.at("truelabel").get_to(truelabel);
        j.at("label").get_to(label);
        j.at("pred").get_to(pred);
        j.at("root").get_to(root);
        j.at("pathval").get_to(pathval);
        j.at("radius").get_to(radius);
        j.at("dens").get_to(dens);
        j.at("status").get_to(status);
        j.at("relevant").get_to(relevant);
        j.at("nplatadj").get_to(nplatadj);
    }
};