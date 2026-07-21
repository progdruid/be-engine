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
    TickLength: float = 2.0
    AimBoxHalf: float = 1.0
    DashPeriod: float = 2.0
    UiColor: float3 = (0.93, 0.91, 0.84)
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
    float AimBoxHalf;
    float DashPeriod;
    float3 UiColor;
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

    // east/west wing ticks only
    float aimR = _Main.AimRadius / ps;
    if (abs(ad.x - aimR) <= _Main.TickLength && ad.y <= hw) hit = 1.0;

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

    PixelOutput output;
    output.HudOutput = float4(_Main.UiColor, hit);
    return output;
}
