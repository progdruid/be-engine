/*

@be-material: ship-hud-material {
    ScreenSize: float2 = (1280.0, 720.0)
    AimOffset: float2 = (0.0, 0.0)
    AimRadius: float = 150.0
    PixelSize: float = 4.0
    LineHalf: float = 0.0
    PipHalf: float = 0.0
    BracketOffset: float = 6.0
    BracketArm: float = 3.0
    TickLength: float = 3.0
    GroundTickLength: float = 2.0
    AimBoxHalf: float = 1.0
    DashPeriod: float = 2.0
    UiColor: float3 = (0.93, 0.91, 0.84)
    
    TargetPos: float2 = (0.0, 0.0)
    TargetDir: float2 = (0.0, 1.0)
    TargetState: float = 0.0
    TargetRingRadius: float = 5.0
    TargetArrowSize: float = 4.0
    TargetAlpha: float = 1.0
    HorizonDir: float2 = (1.0, 0.0)
}

@be-shader ship-hud {
    topology triangle-strip
    rasterizer back-solid
    blend disable
    depth disable

    vertex FullscreenVertexKernel
    pixel PS

    bind s0 frame uniform-material
    bind s1 main ship-hud-material

    target s0 HudOutput float4
}

*/

/*========================================================*/
// region @be-auto-boilerplate
#include "uniform-material.hlsl"

struct ship_hud_material {
    float2 ScreenSize;
    float2 AimOffset;
    float AimRadius;
    float PixelSize;
    float LineHalf;
    float PipHalf;
    float BracketOffset;
    float BracketArm;
    float TickLength;
    float GroundTickLength;
    float AimBoxHalf;
    float DashPeriod;
    float3 UiColor;
    float2 TargetPos;
    float2 TargetDir;
    float TargetState;
    float TargetRingRadius;
    float TargetArrowSize;
    float TargetAlpha;
    float2 HorizonDir;
};

cbuffer CBuffer_0 : register(b0, space0) {
    uniform_material _Frame;
};

cbuffer CBuffer_1 : register(b0, space1) {
    ship_hud_material _Main;
};

struct PixelOutput {
    float4 HudOutput : SV_Target0;
};

// endregion
/*========================================================*/

#include "fullscreen-vertex.hlsl"

PixelOutput PS(FullscreenVSOutput input) {
    float ps = _Main.PixelSize;
    float hw = _Main.LineHalf;

    float2 center = _Main.ScreenSize * 0.5;
    float2 aimPos = center + _Main.AimOffset * _Main.AimRadius;

    // everything measured in integer cell offsets from the center cell
    float2 cell = floor(input.Position.xy / ps);
    float2 c0 = floor(center / ps);
    float2 d = cell - c0;
    float2 ad = abs(d);

    float hit = 0.0;

    // center pip
    if (max(ad.x, ad.y) <= _Main.PipHalf) hit = 1.0;

    // corner-bracket boresight (arms point inward toward the pip)
    float bd = _Main.BracketOffset;
    float arm = _Main.BracketArm;
    bool hArm = abs(ad.y - bd) <= hw && ad.x <= bd + hw && ad.x >= bd - arm;
    bool vArm = abs(ad.x - bd) <= hw && ad.y <= bd + hw && ad.y >= bd - arm;
    if (hArm || vArm) hit = 1.0;

    // horizon-aligned wing ticks (bar stays parallel to the world horizon)
    float aimR = _Main.AimRadius / ps;
    float2 barDir = _Main.HorizonDir;
    float2 barPerp = float2(-barDir.y, barDir.x);
    float barAlong = dot(d, barDir);
    float barAcross = dot(d, barPerp);
    if (abs(abs(barAlong) - aimR) <= _Main.TickLength && abs(barAcross) <= hw + 0.5) hit = 1.0;

    // ground indicator: drop from each tick's inner corner toward the ground (+barPerp)
    float innerEnd = aimR - _Main.TickLength + 0.5;
    if (abs(abs(barAlong) - innerEnd) <= 0.5 && barAcross >= 0.5 && barAcross <= _Main.GroundTickLength + 0.5) hit = 1.0;

    // aim marker: solid box, snapped to the cell grid
    float2 aimD = floor(aimPos / ps) - c0;
    float2 da = abs(d - aimD);
    if (max(da.x, da.y) <= _Main.AimBoxHalf) hit = 1.0;

    // dashed leader from boresight to the aim box
    float lenA = length(aimD);
    float tproj = saturate(dot(d, aimD) / max(dot(aimD, aimD), 1e-4));
    float2 closest = aimD * tproj;
    float along = tproj * lenA;
    bool inRange = along > bd + hw + 1.0 && along < lenA - (_Main.AimBoxHalf + 1.0);
    bool dashOn = fmod(floor(along / _Main.DashPeriod), 2.0) < 0.5;
    if (length(d - closest) <= hw + 0.5 && inRange && dashOn) hit = 1.0;

    // delivery target marker: diamond when on-screen, edge chevron when off-screen
    float state = _Main.TargetState;
    if (state > 0.5) {
        float2 tCell = floor(_Main.TargetPos / ps) - c0;
        float2 rel = d - tCell;
        if (state < 1.5) {
            float ring = abs(rel.x) + abs(rel.y);
            if (abs(ring - _Main.TargetRingRadius) <= hw + 0.5) hit = max(hit, _Main.TargetAlpha);
        } else {
            float2 fwd = _Main.TargetDir;
            float2 side = float2(-fwd.y, fwd.x);
            float f = dot(rel, fwd);
            float s = dot(rel, side);
            float sz = _Main.TargetArrowSize;
            float taper = saturate((sz - f) / (sz * 1.5));
            if (f <= sz && f >= -sz * 0.5 && abs(s) <= taper * sz * 0.8) hit = 1.0;
        }
    }

    PixelOutput output;
    output.HudOutput = float4(_Main.UiColor, hit);
    return output;
}
