#pragma once

#include "core/Screen.h"

namespace screens {

// GAME_START: wybor trybu gry (Boxer/Kopacz/...) po wrzuceniu kredytu.
class ModeSelectScreen : public core::Screen {
public:
    void OnEnter(core::AppContext& ctx) override;
    std::optional<core::GameState> HandleEvent(const core::InputEvent& event,
                                               core::AppContext& ctx) override;
    std::optional<core::GameState> Update(core::AppContext& ctx, double dt_ms) override;
    void Render(core::AppContext& ctx) override;

private:
    double idle_ms_ = 0.0;
    static constexpr double kIdleTimeoutMs = 20000.0;
};

}  // namespace screens
