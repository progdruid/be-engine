#include "BeBRPSubmissionBuffer.h"


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


