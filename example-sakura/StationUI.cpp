#include "StationUI.h"

#include <cmath>
#include <vector>

#include <umbrellas/include-libassert.h>

#include "DeliverySystem.h"
#include "imgui/imgui.h"

auto StationUI::Draw(DeliverySystem& delivery, glm::vec3 shipPosition) -> void {
    const int docked = delivery.GetDockedStation();
    be_assert(docked >= 0, "StationUI::Draw: not docked");

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float pad = 16.0f;
    const float gap = 10.0f;
    const float width = 400.0f;
    const float x = viewport->WorkPos.x + viewport->WorkSize.x - width - pad;
    const float fullHeight = viewport->WorkSize.y - pad * 2.0f;
    const float topHeight = (fullHeight - gap) * 0.5f;
    const float bottomHeight = fullHeight - gap - topHeight;
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::SetNextWindowPos(ImVec2(x, viewport->WorkPos.y + pad), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, topHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.82f);
    ImGui::Begin("Station", nullptr, flags);

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

    ImGui::SetNextWindowPos(ImVec2(x, viewport->WorkPos.y + pad + topHeight + gap), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, bottomHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.82f);
    ImGui::Begin("Market", nullptr, flags);

    ImGui::PushTextWrapPos(0.0f);

    ImGui::Text("Cargo: %d / %d t", delivery.GetCargoUsed(), delivery.GetCargoCapacity());
    const bool cargoFull = delivery.GetCargoUsed() >= delivery.GetCargoCapacity();
    const std::vector<DeliverySystem::MarketEntry>& market = delivery.GetStationMarket(docked);

    ImGui::SeparatorText("For Sale Here");
    for (const DeliverySystem::MarketEntry& entry : market) {
        if (!entry.ForSale) continue;
        ImGui::PushID(entry.Commodity);
        ImGui::Text("%s", delivery.GetCommodityName(entry.Commodity).c_str());
        ImGui::SameLine(140.0f);
        ImGui::Text("%d cr/t", entry.Price);
        ImGui::SameLine();
        const float average = delivery.GetCommodityAverage(entry.Commodity);
        const int percent = static_cast<int>(std::lround((entry.Price - average) / average * 100.0f));
        const bool goodDeal = entry.Price < average;
        ImGui::TextColored(goodDeal ? ImVec4(0.45f, 0.85f, 0.45f, 1.0f) : ImVec4(0.9f, 0.55f, 0.45f, 1.0f), "(%+d%%)", percent);
        ImGui::SameLine();
        const bool canBuy = !cargoFull && delivery.GetCredits() >= entry.Price;
        ImGui::BeginDisabled(!canBuy);
        if (ImGui::Button("Buy")) delivery.BuyCommodity(docked, entry.Commodity, 1);
        ImGui::EndDisabled();
        ImGui::PopID();
    }

    ImGui::SeparatorText("Your Cargo");
    bool anyCargo = false;
    for (const DeliverySystem::MarketEntry& entry : market) {
        const int held = delivery.GetCargoTons(entry.Commodity);
        if (held <= 0) continue;
        anyCargo = true;
        ImGui::PushID(entry.Commodity);
        ImGui::Text("%s  x%d", delivery.GetCommodityName(entry.Commodity).c_str(), held);
        ImGui::SameLine(140.0f);
        ImGui::Text("%d cr", entry.Price);
        ImGui::SameLine();
        const float average = delivery.GetCommodityAverage(entry.Commodity);
        const int percent = static_cast<int>(std::lround((entry.Price - average) / average * 100.0f));
        const bool goodDeal = entry.Price > average;
        ImGui::TextColored(goodDeal ? ImVec4(0.45f, 0.85f, 0.45f, 1.0f) : ImVec4(0.9f, 0.55f, 0.45f, 1.0f), "(%+d%%)", percent);
        ImGui::SameLine();
        if (ImGui::Button("Sell")) delivery.SellCommodity(docked, entry.Commodity, 1);
        ImGui::PopID();
    }
    if (!anyCargo) ImGui::TextDisabled("Cargo hold is empty.");

    ImGui::PopTextWrapPos();

    ImGui::End();
}
