#include <opf/file.hpp>
#include <opf/common.hpp>
#include <cstdio>
#include <vector>

namespace opf {

template<typename T>
void readSubgraph(const std::string& filename, Subgraph<T>& sg) {
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        Error("Cannot open file", "readSubgraph");
    }

    int n, nlabels, nfeats;
    fread(&n, sizeof(int), 1, fp);
    fread(&nlabels, sizeof(int), 1, fp);
    fread(&nfeats, sizeof(int), 1, fp);

    sg.setNumLabels(nlabels);
    sg.setNumFeats(nfeats);

    std::vector<Node<T>> nodes(n);
    for (int i = 0; i < n; ++i) {
        int truelabel, position;
        float pathval;
        fread(&truelabel, sizeof(int), 1, fp);
        fread(&position, sizeof(int), 1, fp);
        fread(&pathval, sizeof(float), 1, fp);

        nodes[i].setTruelabel(truelabel);
        nodes[i].setPosition(position);
        nodes[i].setPathval(pathval);

        auto feat = std::make_shared<std::vector<T>>(nfeats);
        fread(feat->data(), sizeof(T), nfeats, fp);
        nodes[i].setFeat(feat);
    }
    sg.setNodes(nodes);

    fclose(fp);
}

template void readSubgraph<float>(const std::string& filename, Subgraph<float>& sg);
template void readSubgraph<double>(const std::string& filename, Subgraph<double>& sg);

template<typename T>
void writeSubgraph(const std::string& filename, const Subgraph<T>& sg) {
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        Error("Cannot open file", "writeSubgraph");
    }

    int n = sg.getNumNodes();
    int nlabels = sg.getNumLabels();
    int nfeats = sg.getNumFeats();

    fwrite(&n, sizeof(int), 1, fp);
    fwrite(&nlabels, sizeof(int), 1, fp);
    fwrite(&nfeats, sizeof(int), 1, fp);

    for (int i = 0; i < n; ++i) {
        int truelabel = sg.getNode(i).getTruelabel();
        int position = sg.getNode(i).getPosition();
        float pathval = sg.getNode(i).getPathval();

        fwrite(&truelabel, sizeof(int), 1, fp);
        fwrite(&position, sizeof(int), 1, fp);
        fwrite(&pathval, sizeof(float), 1, fp);

        fwrite(sg.getNode(i).getFeat()->data(), sizeof(T), nfeats, fp);
    }

    fclose(fp);
}

template void writeSubgraph<float>(const std::string& filename, const Subgraph<float>& sg);
template void writeSubgraph<double>(const std::string& filename, const Subgraph<double>& sg);

} // namespace opf
