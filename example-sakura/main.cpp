#include "Game.h"

#include <filesystem>

int main(int argc, char** argv) {
    if (argc > 0 && argv[0]) {
        auto dir = std::filesystem::path(argv[0]).parent_path();
        for (int i = 0; i < 5 && !dir.empty(); i++) {
            if (std::filesystem::exists(dir / "assets" / "shaders")) {
                std::filesystem::current_path(dir);
                break;
            }
            dir = dir.parent_path();
        }
    }

    const auto game = new Game();
    const auto result = game->Run();
    delete game;

    if (result != 0) {
        return 1;
    }
    
    return 0;
}
