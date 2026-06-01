#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include <opf/KernelSubGraph.hpp>
#include <opf/Subgraph.hpp>

int main() {
    opf::Subgraph<float> sg(3);
    sg.setNumFeats(5);

    for (int i = 0; i < sg.getNumNodes(); ++i) {
        auto feat = std::make_shared<std::vector<float>>(5);
        for (int j = 0; j < 5; ++j) {
            (*feat)[j] = static_cast<float>(i * 10 + j);
        }
        sg.getNode(i).setFeat(feat);

        // Parent metadata position in arbitrary [1..n] order.
        const int parent_positions[3] = {3, 1, 2};
        sg.getNode(i).setPosition(parent_positions[i]);
    }

    const std::vector<std::pair<int, int>> slices = {
        {0, 2},
        {2, 3}
    };

    auto kernels = opf::splitSubgraphIntoKernels(sg, slices);

    assert(kernels.size() == 2);
    assert(kernels[0].getNumFeats() == 2);
    assert(kernels[1].getNumFeats() == 3);

    for (const auto& kernel : kernels) {
        assert(kernel.getNumNodes() == sg.getNumNodes());
        for (int i = 0; i < kernel.getNumNodes(); ++i) {
            // Position must preserve parent sg node position metadata.
            assert(kernel.getNode(i).getPosition() == sg.getNode(i).getPosition());
        }
    }

    std::cout << "OK: kernel node positions preserve parent sg positions." << std::endl;
    return 0;
}
