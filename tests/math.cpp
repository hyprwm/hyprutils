#include <hyprutils/math/Region.hpp>
#include "shared.hpp"

using namespace Hyprutils::Math;

int main(int argc, char** argv, char** envp) {
    CRegion rg = {0, 0, 100, 100};
    rg.add(CBox{{}, {20, 200}});

    int ret = 0;

    EXPECT(rg.getExtents().height, 200);
    EXPECT(rg.getExtents().width, 100);
    
    rg.intersect(CBox{10, 10, 300, 300});

    EXPECT(rg.getExtents().width, 90);
    EXPECT(rg.getExtents().height, 190);

    return ret;
}