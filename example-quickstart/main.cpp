#include <quickstart/BeQuickstart.h>

#include "BeMaterial.h"
#include "BeMeshPrimitives.h"

int main() {
    BeQuickstart quickstart;
    quickstart.Title = "be: quickstart";
    quickstart.SkyHdrPath = "assets/kloofendal_puresky.hdr";
    quickstart.AmbientColor = glm::vec3(0.0f);

    std::shared_ptr<BeProp> cube;
    std::shared_ptr<BeProp> floor;

    quickstart.OnStart = [&] {
        cube = quickstart.CreateProp(BeMeshPrimitives::Cube());
        cube->Materials[0]->SetFloat3("BaseColor", HexColor("#E9A17C"));
        cube->Materials[0]->SetFloat1("Roughness", 0.35f);

        floor = quickstart.CreateProp(BeMeshPrimitives::Plane());
        floor->Materials[0]->SetFloat3("BaseColor", HexColor("#3B4A5A"));

        quickstart.Camera->Position = { 0.0f, 2.5f, -6.0f };
    };

    quickstart.OnTick = [&](float deltaTime) {
        quickstart.RenderProp("floor", floor, { 0.0f, -1.0f, 0.0f }, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(30.0f));
        quickstart.RenderProp("cube", cube, { 0.0f, 0.0f, 0.0f }, glm::quat(glm::vec3(0.0f, quickstart.Time, 0.0f)));

        quickstart.RenderSunLight({
            .Direction = { -0.5f, -1.0f, 0.3f },
            .Color = HexColor("#FFF2D8"),
            .Power = 3.0f,
        });

        quickstart.RenderPointLight({
            .Name = "orbiting",
            .Position = { 3.0f * std::cos(quickstart.Time), 1.5f, 3.0f * std::sin(quickstart.Time) },
            .Radius = 12.0f,
            .Color = HexColor("#7CC7E9"),
            .Power = 8.0f,
        });
    };

    return quickstart.Run();
}
