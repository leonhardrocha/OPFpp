#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include <opf/KernelLayout.hpp>
#include <opf/OPFpp.hpp>
#include <opf/StridedSubgraph.hpp>

int main() {
    opf::Subgraph<float> sg(12);
    sg.setNumFeats(6);
    sg.setNumLabels(0);

    for (int i = 0; i < sg.getNumNodes(); ++i) {
        auto feat = std::make_shared<std::vector<float>>(6);
        const float base = (i < 6) ? 0.0f : 5.0f;
        for (int j = 0; j < 6; ++j) {
            (*feat)[j] = base + static_cast<float>(j) * 0.02f + static_cast<float>(i % 3) * 0.01f;
        }
        sg.getNode(i).setFeat(feat);
        sg.getNode(i).setPosition(100 + i);
        sg.getNode(i).setLabel(0);
        sg.getNode(i).setTruelabel(0);
    }

    opf::KernelLayout layout({
        opf::KernelSlice{0, 0, 2},
        opf::KernelSlice{1, 2, 2},
        opf::KernelSlice{2, 4, 2},
    });
    opf::StridedSubgraph<float> strided(sg, layout);

    opf::OPFpp<float> clf;
    auto results = clf.bestkMinCutPerStrideKernel(strided, 2, 4);

    assert(results.size() == 3);
    for (size_t i = 0; i < results.size(); ++i) {
        assert(results[i].kernel_id == static_cast<int>(i));
        assert(results[i].bestk >= 2);
        assert(results[i].bestk <= 4);
        assert(results[i].df_at_bestk > 0.0f);
    }

    std::cout << "OK: OPF stride best-k path validated." << std::endl;
    return 0;
}
