#pragma once

#include <memory>
#include <vector>

class BeRenderPass;

struct BePassSequence {
    std::vector<std::unique_ptr<BeRenderPass>> Passes;
};
