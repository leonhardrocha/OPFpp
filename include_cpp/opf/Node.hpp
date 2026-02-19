#ifndef OPF_NODE_HPP
#define OPF_NODE_HPP

#include <opf/common.hpp>

namespace opf {
    template<typename T>
    class Node {
    public:
        // Default constructor
        Node() : pathval(0.0f), dens(0.0f), radius(0.0f), label(0), root(0), pred(0),
            truelabel(0), position(0), status(0), relevant(0), nplatadj(0) {}

        // Getters
        float getPathval() const { return pathval; }
        float getDens() const { return dens; }
        float getRadius() const { return radius; }
        int getLabel() const { return label; }
        int getRoot() const { return root; }
        int getPred() const { return pred; }
        int getTruelabel() const { return truelabel; }
        int getPosition() const { return position; }
        uint8_t getStatus() const { return status; }
        uint8_t getRelevant() const { return relevant; }
        int getNplatadj() const { return nplatadj; }
        const std::shared_ptr<std::vector<T>>& getFeat() const { return feat; }
        const std::vector<int>& getAdj() const { return adj; }
        std::vector<int>& getAdj() { return adj; }

        // Setters
        void setPathval(float val) { pathval = val; }
        void setDens(float val) { dens = val; }
        void setRadius(float val) { radius = val; }
        void setLabel(int val) { label = val; }
        void setRoot(int val) { root = val; }
        void setPred(int val) { pred = val; }
        void setTruelabel(int val) { truelabel = val; }
        void setPosition(int val) { position = val; }
        void setStatus(uint8_t val) { status = val; }
        void setRelevant(uint8_t val) { relevant = val; }
        void setNplatadj(int val) { nplatadj = val; }
        void setFeat(const std::shared_ptr<std::vector<T>>& val) { feat = val; }
        
        // Adjacency list manipulation
        void addToAdj(int node_index) { adj.push_back(node_index); }
        void clearAdj() { adj.clear(); }

    private:
        float pathval = 0.0f;
        float dens = 0.0f;
        float radius = 0.0f;
        int label = 0;
        int root = 0;
        int pred = 0;
        int truelabel = 0;
        int position = 0;
        uint8_t status = 0;
        uint8_t relevant = 0;
        int nplatadj = 0;

        std::shared_ptr<std::vector<T>> feat;
        std::vector<int> adj;
    };
} // namespace opf

#endif // OPF_NODE_HPP
