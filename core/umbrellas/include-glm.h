#pragma once

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define NOMINMAX

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

constexpr float operator""_rad(long double deg) {
    return static_cast<float>(deg * 0.01745329251994329576923690768489);
}

constexpr float operator""_rad(unsigned long long deg) {
    return static_cast<float>(deg * 0.01745329251994329576923690768489);
}

inline glm::vec3 HexColor(const char* hex) {
    if (hex[0] == '#') hex++;
    unsigned int r, g, b;
    r = (hex[0] <= '9' ? hex[0] - '0' : (hex[0] | 0x20) - 'a' + 10) * 16
      + (hex[1] <= '9' ? hex[1] - '0' : (hex[1] | 0x20) - 'a' + 10);
    g = (hex[2] <= '9' ? hex[2] - '0' : (hex[2] | 0x20) - 'a' + 10) * 16
      + (hex[3] <= '9' ? hex[3] - '0' : (hex[3] | 0x20) - 'a' + 10);
    b = (hex[4] <= '9' ? hex[4] - '0' : (hex[4] | 0x20) - 'a' + 10) * 16
      + (hex[5] <= '9' ? hex[5] - '0' : (hex[5] | 0x20) - 'a' + 10);
    return glm::vec3(r / 255.f, g / 255.f, b / 255.f);
}