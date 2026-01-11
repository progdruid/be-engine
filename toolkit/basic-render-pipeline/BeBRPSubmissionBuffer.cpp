#include "BeBRPSubmissionBuffer.h"


auto BeBRPGeometryEntry::CalculateModelMatrix(glm::vec3 pos, glm::quat rot, glm::vec3 scale) -> glm::mat4 {
    const glm::mat4x4 modelMatrix =
        glm::translate(glm::mat4(1.0f), pos) *
        glm::mat4_cast(rot) *
        glm::scale(glm::mat4(1.0f), scale);
    return modelMatrix;
}

auto BeBRPSunLightEntry::CalculateViewProj(glm::vec3 direction, float shadowCameraDistance, float shadowMapWorldSize,
                                           float shadowNearPlane, float shadowFarPlane) -> glm::mat4 {
    const float halfSize = shadowMapWorldSize * 0.5f;
    const glm::mat4 lightOrtho = glm::orthoLH_ZO(-halfSize, halfSize, -halfSize, halfSize, shadowNearPlane, shadowFarPlane);
    const glm::vec3 lightPos = -direction * shadowCameraDistance;
    const glm::mat4 lightView = glm::lookAtLH(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    return lightOrtho * lightView;
}

auto BeBRPSubmissionBuffer::ClearEntries() -> void {
    _geometryEntries.clear();
    _sunLightEntries.clear();
    _pointLightEntries.clear();
}

auto BeBRPSubmissionBuffer::SubmitGeometry(const BeBRPGeometryEntry& entry) -> void {
    _geometryEntries.push_back(entry);
}

auto BeBRPSubmissionBuffer::SubmitSunLight(const BeBRPSunLightEntry& entry) -> void {
    _sunLightEntries.push_back(entry);
}

auto BeBRPSubmissionBuffer::SubmitPointLight(const BeBRPPointLightEntry& entry) -> void {
    _pointLightEntries.push_back(entry);
}

auto BeBRPSubmissionBuffer::GetGeometryEntries() const -> const std::vector<BeBRPGeometryEntry>& {
    return _geometryEntries;
}

auto BeBRPSubmissionBuffer::GetSunLightEntries() const -> const std::vector<BeBRPSunLightEntry>& {
    return _sunLightEntries;
}

auto BeBRPSubmissionBuffer::GetPointLightEntries() const -> const std::vector<BeBRPPointLightEntry>& {
    return _pointLightEntries;
}


