#include <memory>

#include "standard-game/BeStandardGame.h"
#include "standard-game/BeStandardBaseScene.h"
#include "scenes/BeSceneManager.h"

#include "scenes/ShowcaseScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SakuraScene.h"
#include "scenes/OldScene.h"

int main() {
    BeStandardGame game({
        .Title = "be: example sakura",
        .WindowMode = BeWindowMode::Fullscreen,
        .Width = 800,
        .Height = 600,
    });

    auto& scenes = *game.SceneManager;
    scenes.RegisterScene("menu", std::make_unique<MenuScene>(&game));
    scenes.RegisterScene("sakura", std::make_unique<SakuraScene>(&game));
    scenes.RegisterScene("showcase", std::make_unique<ShowcaseScene>(&game));
    scenes.RegisterScene("old", std::make_unique<OldScene>(&game));

    scenes.GetScene<BeStandardBaseScene>("menu")->Prepare();
    scenes.GetScene<BeStandardBaseScene>("sakura")->Prepare();
    scenes.GetScene<BeStandardBaseScene>("showcase")->Prepare();
    scenes.GetScene<BeStandardBaseScene>("old")->Prepare();

    scenes.RequestSceneChange("menu");
    scenes.ApplyPendingSceneChange();

    return game.Run();
}
