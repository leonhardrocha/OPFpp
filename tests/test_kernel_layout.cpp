#include <cassert>
#include <iostream>
#include <vector>

#include <opf/KernelLayout.hpp>

int main() {
    std::vector<opf::KernelSlice> slices = {
        {10, 0, 3},
        {20, 3, 2},
        {30, 5, 4},
    };

    opf::KernelLayout layout(slices);

    assert(layout.numKernels() == 3);
    assert(layout.hasKernel(10));
    assert(layout.hasKernel(20));
    assert(layout.hasKernel(30));
    assert(!layout.hasKernel(40));

    assert(layout.offset(10) == 0);
    assert(layout.size(20) == 2);
    assert(layout.end(30) == 9);

    layout.validateAgainstFeatureCount(9);

    std::vector<int> ids = layout.orderedKernelIds();
    assert(ids.size() == 3);
    assert(ids[0] == 10);
    assert(ids[1] == 20);
    assert(ids[2] == 30);

    std::cout << "OK: KernelLayout basic behavior validated." << std::endl;
    return 0;
}
