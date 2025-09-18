#include "Program.h"

int main() {

    const auto program = new Program();
    const auto result = program->run();
    delete program;

    if (result != 0) {
        return 1;
    }
    
    return 0;
}
