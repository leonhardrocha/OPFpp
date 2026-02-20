#include <opf/OPF.hpp>
#include <opf/common.hpp>
#include <opf/Distance.hpp>
#include <queue>
#include <vector>
#include <cfloat>

namespace opf {

template<typename T>
void OPF<T>::classify(const Subgraph<T>& sg_train, Subgraph<T>& sg_test) {
    for (int i = 0; i < sg_test.getNumberOfNodes(); ++i) {
        int j = 0;
        int k = sg_train.getOrderedListOfNodes()[j];
        float weight = distance::euclDist(*sg_train.getNode(k).getFeat(), *sg_test.getNode(i).getFeat());
        float min_cost = std::max(sg_train.getNode(k).getPathval(), weight);
        int label = sg_train.getNode(k).getLabel();

        while (j < sg_train.getNumberOfNodes() - 1 && min_cost > sg_train.getNode(sg_train.getOrderedListOfNodes()[j + 1]).getPathval()) {
            int l = sg_train.getOrderedListOfNodes()[j + 1];
            weight = distance::euclDist(*sg_train.getNode(l).getFeat(), *sg_test.getNode(i).getFeat());
            float tmp = std::max(sg_train.getNode(l).getPathval(), weight);
            if (tmp < min_cost) {
                min_cost = tmp;
                label = sg_train.getNode(l).getLabel();
            }
            j++;
        }
        sg_test.getNode(i).setLabel(label);
    }
}

void OPF<T>::train(Subgraph<T>& sg) {
    mstPrototypes(sg);

    int nnodes = sg.getNumberOfNodes();
    std::vector<float> pathval(nnodes, FLT_MAX);
    
    using Elem = std::pair<float, int>;
    std::priority_queue<Elem, std::vector<Elem>, std::greater<Elem>> Q;

    for (int p = 0; p < nnodes; ++p) {
        if (sg.getNode(p).getStatus() == 1) { // opf_PROTOTYPE
            sg.getNode(p).setPred(NIL);
            pathval[p] = 0;
            sg.getNode(p).setLabel(sg.getNode(p).getTruelabel());
            Q.push({0.0f, p});
        }
    }

    std::vector<int> ordered_list_of_nodes(nnodes);
    int i = 0;

    while (!Q.empty()) {
        int p = Q.top().second;
        Q.pop();

        ordered_list_of_nodes[i++] = p;
        sg.getNode(p).setPathval(pathval[p]);

        for (int q = 0; q < nnodes; ++q) {
            if (p != q) {
                if (pathval[p] < pathval[q]) {
                    float weight = distance::euclDist(*sg.getNode(p).getFeat(), *sg.getNode(q).getFeat());
                    float tmp = std::max(pathval[p], weight);
                    if (tmp < pathval[q]) {
                        sg.getNode(q).setPred(p);
                        sg.getNode(q).setLabel(sg.getNode(p).getLabel());
                        pathval[q] = tmp;
                        Q.push({tmp, q});
                    }
                }
            }
        }
    }
    sg.setOrderedListOfNodes(ordered_list_of_nodes);
}

template<typename T>
void OPF<T>::mstPrototypes(Subgraph<T>& sg) {
    int nnodes = sg.getNumberOfNodes();
    std::vector<float> pathval(nnodes, FLT_MAX);
    
    using Elem = std::pair<float, int>;
    std::priority_queue<Elem, std::vector<Elem>, std::greater<Elem>> Q;

    for (int i = 0; i < nnodes; ++i) {
        sg.getNode(i).setStatus(0);
    }

    pathval[0] = 0;
    sg.getNode(0).setPred(NIL);
    Q.push({0.0f, 0});

    while (!Q.empty()) {
        int p = Q.top().second;
        Q.pop();

        int pred = sg.getNode(p).getPred();
        if (pred != NIL) {
            if (sg.getNode(p).getTruelabel() != sg.getNode(pred).getTruelabel()) {
                if (sg.getNode(p).getStatus() != 1) {
                    sg.getNode(p).setStatus(1);
                }
                if (sg.getNode(pred).getStatus() != 1) {
                    sg.getNode(pred).setStatus(1);
                }
            }
        }

        for (int q = 0; q < nnodes; ++q) {
            if (p != q) {
                float weight = distance::euclDist(*sg.getNode(p).getFeat(), *sg.getNode(q).getFeat());
                if (weight < pathval[q]) {
                    sg.getNode(q).setPred(p);
                    pathval[q] = weight;
                    Q.push({weight, q});
                }
            }
        }
    }
}

template class OPF<float>;
template class OPF<double>;

} // namespace opf
