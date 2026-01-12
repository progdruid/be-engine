#pragma once

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

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