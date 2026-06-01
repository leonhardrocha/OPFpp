#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include <opf/KernelLayout.hpp>
#include <opf/StridedSubgraph.hpp>
#include <opf/Subgraph.hpp>

int main() {
    opf::Subgraph<float> sg(4);
    sg.setNumFeats(6);

    const int parent_positions[4] = {42, 7, 101, 9};
    for (int i = 0; i < sg.getNumNodes(); ++i) {
        auto feat = std::make_shared<std::vector<float>>(6);
        for (int j = 0; j < 6; ++j) {
            (*feat)[j] = static_cast<float>(i * 100 + j);
        }
        sg.getNode(i).setFeat(feat);
        sg.getNode(i).setPosition(parent_positions[i]);
        sg.getNode(i).setDens(static_cast<float>(i + 1));
    }

    opf::KernelLayout layout({
        opf::KernelSlice{0, 0, 2},
        opf::KernelSlice{1, 2, 2},
        opf::KernelSlice{2, 4, 2},
    });

    opf::StridedSubgraph<float> strided(sg, layout);

    assert(strided.getNumKernels() == 3);
    assert(strided.getNumNodes() == 4);
    assert(strided.getKernelOffset(1) == 2);
    assert(strided.getKernelSize(2) == 2);

    for (int kernel_id = 0; kernel_id < strided.getNumKernels(); ++kernel_id) {
        for (int node_idx = 0; node_idx < strided.getNumNodes(); ++node_idx) {
            auto node = strided.getNode(kernel_id, node_idx);
            assert(node.getPosition() == parent_positions[node_idx]);

            std::vector<float> slice = node.getFeatSlice();
            assert(slice.size() == 2);
            assert(slice[0] == static_cast<float>(node_idx * 100 + strided.getKernelOffset(kernel_id)));
            assert(slice[1] == static_cast<float>(node_idx * 100 + strided.getKernelOffset(kernel_id) + 1));
        }
    }

    std::vector<float> probs = strided.computeLogDensityProbabilities(0);
    assert(probs.size() == 4);
    assert(probs[0] == 0.0f);
    assert(probs[1] > 0.0f);

    opf::Subgraph<float> kernel_copy = strided.makeKernelCopy(1);
    assert(kernel_copy.getNumNodes() == sg.getNumNodes());
    assert(kernel_copy.getNumFeats() == 2);
    for (int i = 0; i < kernel_copy.getNumNodes(); ++i) {
        assert(kernel_copy.getNode(i).getPosition() == parent_positions[i]);
        const auto& copied_feat = *kernel_copy.getNode(i).getFeat();
        assert(copied_feat.size() == 2);
        assert(copied_feat[0] == static_cast<float>(i * 100 + 2));
        assert(copied_feat[1] == static_cast<float>(i * 100 + 3));
    }

    std::cout << "OK: StridedSubgraph preserves parent positions and kernel slices." << std::endl;
    return 0;
}
