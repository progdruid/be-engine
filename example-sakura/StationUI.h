#pragma once

#include <umbrellas/include-glm.h>

class DeliverySystem;

namespace StationUI {
    auto Draw(DeliverySystem& delivery, glm::vec3 shipPosition) -> void;
}
