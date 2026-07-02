#include <cassert>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <opf/KernelSubGraph.hpp>
#include <opf/Subgraph.hpp>

/// Test: Verify that kernel node positions enable correct mapping back to original sg.
int main() {
    // === Setup: create sg with nfeats=10, nnodes=4 ===
    opf::Subgraph<float> sg(4);
    sg.setNumFeats(10);

    // External data indexed by parent position (arbitrary ids, not node order).
    std::unordered_map<int, float> external_value_by_position;
    const int parent_positions[4] = {42, 7, 101, 9};

    for (int i = 0; i < sg.getNumNodes(); ++i) {
        auto feat = std::make_shared<std::vector<float>>(10);
        for (int j = 0; j < 10; ++j) {
            // Feature value: node_idx * 1000 + feature_idx
            (*feat)[j] = static_cast<float>(i * 1000 + j);
        }
        sg.getNode(i).setFeat(feat);
        sg.getNode(i).setPosition(parent_positions[i]);

        // Simulate external payload keyed by parent position.
        external_value_by_position[parent_positions[i]] = 10000.0f + static_cast<float>(i);
    }

    // === Split into 2 kernels: features [0,5) and [5,10) ===
    const std::vector<std::pair<int, int>> slices = {
        {0, 5},   // kernel 0: features 0..4
        {5, 10}   // kernel 1: features 5..9
    };

    auto kernels = opf::splitSubgraphIntoKernels(sg, slices);

    assert(kernels.size() == 2);
    assert(kernels[0].getNumFeats() == 5);
    assert(kernels[1].getNumFeats() == 5);

    // === Validate: kernel nodes preserve parent external index (position) and slices ===
    for (int k = 0; k < 2; ++k) {
        std::cout << "Kernel " << k << ":" << std::endl;
        const auto& kernel = kernels[k];

        for (int node_idx = 0; node_idx < kernel.getNumNodes(); ++node_idx) {
            const auto& kn = kernel.getNode(node_idx);

            // Child kernel must keep exactly the same external index as parent sg node.
            int parent_pos = sg.getNode(node_idx).getPosition();
            assert(kn.getPosition() == parent_pos);
            assert(external_value_by_position.count(kn.getPosition()) == 1);

            std::cout << "  node " << node_idx << " -> parent position " << parent_pos
                      << " -> external payload " << external_value_by_position[kn.getPosition()]
                      << std::endl;

            // Verify: the sliced features in kernel match the features in sg.
            const auto& kernel_feat = *kn.getFeat();
            const auto& full_feat = *sg.getNode(node_idx).getFeat();

            int feat_start = slices[k].first;
            int feat_end = feat_start + kernel.getNumFeats();

            // Extract the expected slice from sg features.
            std::vector<float> expected_slice(
                full_feat.begin() + feat_start,
                full_feat.begin() + feat_end
            );

            // Verify kernel features match the expected slice.
            assert(kernel_feat.size() == expected_slice.size());
            for (size_t j = 0; j < kernel_feat.size(); ++j) {
                float expected_val = static_cast<float>(node_idx * 1000 + feat_start + j);
                assert(kernel_feat[j] == expected_val);
                assert(kernel_feat[j] == expected_slice[j]);
            }
        }
    }

    std::cout << "OK: kernel node positions preserve external parent indexing and slices are correct."
              << std::endl;
    return 0;
}
