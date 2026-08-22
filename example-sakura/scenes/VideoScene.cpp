#include "VideoScene.h"

#include <umbrellas/include-glfw.h>

#include "BeInput.h"
#include "BeMeshPrimitives.h"
#include "BeProp.h"
#include "BeShaderLibrary.h"
#include "BeTexture.h"
#include "Game.h"
#include "scenes/BeSceneManager.h"
#include "standard-render-machine/BeStandardRenderMachine.h"

VideoScene::VideoScene(Game* game) : FullScene(game) {}
VideoScene::~VideoScene() {}

void VideoScene::Tick(float deltaTime) {
    FullScene::Tick(deltaTime);
    
    if (_gameIns->Input->GetKeyDown(GLFW_KEY_ESCAPE)) {
        _gameIns->Input->SetMouseCapture(false);
        _gameIns->SceneManager->RequestSceneChange("menu");
        return;
    }
}

auto VideoScene::DefineAssets() -> void {
    _machine->DeclareGBufferTarget("Video_Albedo_RGB",      SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Video_WorldNormal_XYZ", SenFormat::RGBA16_Float);
    _machine->DeclareGBufferTarget("Video_ORM_RGB",         SenFormat::RGBA8_Unorm);
    _machine->DeclareGBufferTarget("Video_Emissive_RGB",    SenFormat::R11G11B10_Float);
    _machine->DeclareGBufferTarget("Video_Other",           SenFormat::R11G11B10_Float);
    _machine->DeclareDepthTarget  ("Video_Depth",           SenFormat::Depth32);
    _machine->DeclareTextureTarget("Video_HDR",             SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Video_Bloom",           SenFormat::R11G11B10_Float);
    _machine->DeclareTextureTarget("Video_Tonemapped",      SenFormat::R11G11B10_Float);

    const auto shader = BeShaderLibrary::GetShader("standard-pbr");

    const auto sphere = BeProp::FromMesh(BeMeshPrimitives::Sphere(), shader, "geometry-main");
    _assetRegistry.AddProp("sphere", sphere);
    _machine->RegisterMesh(sphere->Mesh);

    const auto cube = BeProp::FromMesh(BeMeshPrimitives::Cube(), shader, "geometry-main");
    _assetRegistry.AddProp("cube", cube);
    _machine->RegisterMesh(cube->Mesh);

    _machine->BakeMeshes();
}

auto VideoScene::DefinePasses() -> void {
    _machine->ClearPasses();

    const auto skyTexture = 
        BeTexture::Create("sky-texture")
        .LoadFromFileHdr("assets/moonrise_puresky.hdr")
        .Build()
    ;
    _machine->AddEnvironmentBakePass(skyTexture);
    _machine->BakeEnvironment();

    _machine->AddShadowPass();
    _machine->AddGeometryPass();
    _machine->AddLightingPass("Video_HDR");
    _machine->AddSkyboxPass("Video_HDR");
    _machine->AddBloomPass(5, "Video_HDR", "Video_Bloom");

    _machine->AddTonemapperPass("Video_Bloom", "Video_Tonemapped");
    _machine->AddBackbufferPass("Video_Tonemapped", glm::vec3(0.f));

    _machine->InitialisePasses();
}

