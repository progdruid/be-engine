#include "StationUI.h"

#include <vector>

#include <umbrellas/include-libassert.h>

#include "DeliverySystem.h"
#include "imgui/imgui.h"

auto StationUI::Draw(DeliverySystem& delivery, glm::vec3 shipPosition) -> void {
    const int docked = delivery.GetDockedStation();
    be_assert(docked >= 0, "StationUI::Draw: not docked");

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float pad = 16.0f;
    const float width = 340.0f;
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - width - pad, viewport->WorkPos.y + pad),
        ImGuiCond_Always
    );
    ImGui::SetNextWindowSize(ImVec2(width, viewport->WorkSize.y - pad * 2.0f), ImGuiCond_Always);
    ImGui::Begin("Station", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::PushTextWrapPos(0.0f);

    ImGui::Text("%s", delivery.GetStationName(docked).c_str());
    ImGui::Text("Credits: %d cr", delivery.GetCredits());

    ImGui::SeparatorText("Active Contract");
    if (delivery.HasContract()) {
        const DeliverySystem::Job& contract = *delivery.GetContract();
        ImGui::Text("Deliver to %s", delivery.GetStationName(contract.Destination).c_str());
        ImGui::Text("%.0f m    %d cr", delivery.GetDistanceToTarget(shipPosition), contract.Reward);
        const bool canComplete = delivery.CanComplete();
        ImGui::BeginDisabled(!canComplete);
        if (ImGui::Button("Complete Delivery", ImVec2(-1.0f, 0.0f))) delivery.CompleteContract();
        ImGui::EndDisabled();
        if (!canComplete) ImGui::TextDisabled("Fly to the destination to complete.");
    } else {
        ImGui::TextDisabled("No active contract.");
    }

    ImGui::SeparatorText("Jobs at this Station");
    const std::vector<DeliverySystem::Job>& jobs = delivery.GetStationJobs(docked);
    for (int index = 0; index < static_cast<int>(jobs.size()); ++index) {
        ImGui::PushID(index);
        ImGui::Text("Deliver to %s", delivery.GetStationName(jobs[index].Destination).c_str());
        ImGui::Text("%.0f m    %d cr", jobs[index].Distance, jobs[index].Reward);
        ImGui::SameLine();
        if (ImGui::Button("Take")) delivery.TakeJob(docked, index);
        ImGui::Separator();
        ImGui::PopID();
    }

    ImGui::PopTextWrapPos();

    ImGui::End();
}
