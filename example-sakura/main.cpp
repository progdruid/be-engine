#include "Game.h"

int main() {

    const auto game = new Game();
    const auto result = game->Run();
    delete game;

    if (result != 0) {
        return 1;
    }
    
    return 0;
}
