#include "MetaSystem.h"

#include <cstdio>

#include <umbrellas/include-glm.h>

#include "DeliverySystem.h"
#include "RiftSettings.h"
#include "imgui/imgui.h"

auto MetaSystem::Begin() -> void {
    _phase = Phase::Briefing;
    _popupOpenedFor = Phase::Inactive;
    _elapsed = 0.0f;
    _wantsClose = false;
}

auto MetaSystem::End() -> void {
    _phase = Phase::Inactive;
    _popupOpenedFor = Phase::Inactive;
}

auto MetaSystem::IsPaused() const -> bool {
    return _phase == Phase::Briefing || _phase == Phase::Won || _phase == Phase::Lost;
}

auto MetaSystem::SetElapsed(float elapsed) -> void {
    _elapsed = glm::max(0.0f, elapsed);
}

auto MetaSystem::Update(float deltaTime, const DeliverySystem& delivery) -> void {
    if (_phase != Phase::Running) return;
    const auto& debt = RiftStore::Get().Debt;
    _elapsed += deltaTime;
    if (delivery.GetCredits() >= debt.Target) _phase = Phase::Won;
    else if (_elapsed >= debt.TimeLimit) _phase = Phase::Lost;
}

auto MetaSystem::DrawUI(const DeliverySystem& delivery) -> void {
    const auto& debt = RiftStore::Get().Debt;

    if (_phase == Phase::Running) {
        const ImGuiViewport* barViewport = ImGui::GetMainViewport();
        const float width = 360.0f;
        ImGui::SetNextWindowPos(ImVec2(barViewport->WorkPos.x + 16.0f, barViewport->WorkPos.y + 16.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        const ImGuiWindowFlags barFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs;
        ImGui::Begin("Debt", nullptr, barFlags);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));

        const int credits = delivery.GetCredits();
        const float debtFraction = glm::clamp(static_cast<float>(credits) / static_cast<float>(debt.Target), 0.0f, 1.0f);
        char debtLabel[64];
        std::snprintf(debtLabel, sizeof(debtLabel), "%d / %d cr", credits, debt.Target);
        ImGui::TextUnformatted("Debt");
        ImGui::SameLine(64.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.55f, 0.62f, 0.68f, 0.45f));
        ImGui::ProgressBar(debtFraction, ImVec2(-1.0f, 0.0f), debtLabel);
        ImGui::PopStyleColor();

        const float remaining = glm::max(0.0f, debt.TimeLimit - _elapsed);
        const float timeFraction = glm::clamp(_elapsed / debt.TimeLimit, 0.0f, 1.0f);
        char timeLabel[64];
        std::snprintf(timeLabel, sizeof(timeLabel), "%d:%02d", static_cast<int>(remaining) / 60, static_cast<int>(remaining) % 60);
        ImGui::TextUnformatted("Time");
        ImGui::SameLine(64.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.62f, glm::mix(0.55f, 0.32f, timeFraction), 0.4f, 0.45f));
        ImGui::ProgressBar(timeFraction, ImVec2(-1.0f, 0.0f), timeLabel);
        ImGui::PopStyleColor();

        ImGui::PopStyleColor(2);
        ImGui::End();
    }

    const bool dialogPhase = _phase == Phase::Briefing || _phase == Phase::Won || _phase == Phase::Lost;
    if (!dialogPhase) {
        _popupOpenedFor = Phase::Inactive;
        return;
    }
    if (_popupOpenedFor != _phase) {
        ImGui::OpenPopup("Announcement");
        _popupOpenedFor = _phase;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(460.0f, 0.0f), ImGuiCond_Always);
    const ImGuiWindowFlags dialogFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    if (!ImGui::BeginPopupModal("Announcement", nullptr, dialogFlags)) return;

    ImGui::PushTextWrapPos(0.0f);
    if (_phase == Phase::Briefing) {
        char intro[640];
        std::snprintf(intro, sizeof(intro),
            "Congratulations, Pilot, on the provisional ownership of one (1) "
            "gently pre-owned hauler! Management is delighted to have you aboard "
            "the family.\n\n"
            "A trifling balance of %d cr remains outstanding on the vessel. "
            "Settle it before your repayment window closes and she is yours, "
            "free and clear. Should you fail, we will be obligated to reclaim "
            "the asset, with you still seated in it. Nothing personal. It is "
            "policy.\n\n"
            "Now get out there and move some cargo. Profit is a team sport!",
            debt.Target);
        ImGui::TextUnformatted("WELCOME TO THE FAMILY");
        ImGui::Separator();
        ImGui::TextWrapped("%s", intro);
        ImGui::SeparatorText("Operating Procedures (mandatory)");
        ImGui::TextUnformatted(
            "Mouse    steer\n"
            "W / S    thrust forward / back\n"
            "Q / E    descend / ascend\n"
            "A / D    roll\n"
            "Shift    boost\n"
            "Space    flight assist\n"
            "C        undock");
        ImGui::Spacing();
        if (ImGui::Button("Punch In", ImVec2(-1.0f, 0.0f))) {
            _phase = Phase::Running;
            ImGui::CloseCurrentPopup();
        }
    } else if (_phase == Phase::Won) {
        ImGui::TextUnformatted("ACCOUNT SETTLED");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Well, would you look at that. Balance: zero. The hauler is now "
            "legally, bindingly, and irrevocably yours.\n\n"
            "Management is contractually required to congratulate you, and does "
            "so, briefly, now. Congratulations.\n\n"
            "You owe no one a thing. The stars are open, fly wherever you "
            "please, you magnificent free agent. Do try not to enjoy it too "
            "loudly.");
        ImGui::Spacing();
        if (ImGui::Button("Fly Free", ImVec2(-1.0f, 0.0f))) {
            _phase = Phase::Free;
            ImGui::CloseCurrentPopup();
        }
    } else {
        ImGui::TextUnformatted("ASSET RECLAIMED");
        ImGui::Separator();
        ImGui::TextWrapped(
            "Time's up, and so is your balance -- unfavorably.\n\n"
            "Per subsection 12(b) of an agreement you clicked through without "
            "reading, the vessel and all associated liabilities revert to "
            "Management, effective immediately. Kindly vacate the cockpit. The "
            "airlock is the one marked 'airlock.'\n\n"
            "Thank you for flying with us. We regret that we cannot say the "
            "same.");
        ImGui::Spacing();
        if (ImGui::Button("Eject", ImVec2(-1.0f, 0.0f))) {
            _wantsClose = true;
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::PopTextWrapPos();
    ImGui::EndPopup();
}
