#pragma once

class BeScene {
public:
    virtual ~BeScene() = default;

    virtual auto OnLoad() -> void {}
    virtual auto OnUnload() -> void {}
};
