#include "RailGizmo.h"

#include <string>
#include <umbrellas/include-glm.h>

#include "BeRail.h"
#include "BeProp.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

namespace RailGizmo {
    void DrawRail(
        BeStandardRenderMachine& machine,
        const BeRail& rail,
        const std::shared_ptr<BeProp>& dotProp,
        const std::shared_ptr<BeProp>& knotProp,
        int dotsPerSegment,
        float dotScale,
        float knotScale
    ) {
        const int segments = rail.SegmentCount();
        if (segments <= 0) return;

        // Dots trace the curve between knots.
        for (int i = 0; i < segments * dotsPerSegment; ++i) {
            const float index = static_cast<float>(i) / static_cast<float>(dotsPerSegment);
            machine.AddGeometry({
                .Name = "rig_gizmo_dot_" + std::to_string(i),
                .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(
                    rail.EvalByIndex(index), glm::quat(1, 0, 0, 0), glm::vec3(dotScale)),
                .Prop = dotProp,
                .CastShadows = false,
            });
        }

        // Bigger markers sit on the actual knots (0, 1, 2, ...).
        for (int k = 0; k < segments; ++k) {
            machine.AddGeometry({
                .Name = "rig_gizmo_knot_" + std::to_string(k),
                .ModelMatrix = BeSRMGeometryEntry::CalculateModelMatrix(
                    rail.EvalByIndex(static_cast<float>(k)), glm::quat(1, 0, 0, 0), glm::vec3(knotScale)),
                .Prop = knotProp,
                .CastShadows = false,
            });
        }
    }
}
