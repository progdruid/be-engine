#pragma once

#include <memory>

struct BeProp;
class BeRail;
class BeStandardRenderMachine;

namespace RailGizmo {
    void DrawRail(
        BeStandardRenderMachine& machine,
        const BeRail& rail,
        const std::shared_ptr<BeProp>& dotProp,
        const std::shared_ptr<BeProp>& knotProp,
        int dotsPerSegment = 16,
        float dotScale = 0.2f,
        float knotScale = 0.6f
    );
}
