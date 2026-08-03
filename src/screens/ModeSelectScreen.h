#pragma once

#include "core/Screen.h"

namespace screens {

// GAME_START: czekamy na Start na PGM (gruszka jeszcze u gory).
class ModeSelectScreen : public core::Screen {
public:
    void OnEnter(core::AppContext& ctx) override;
    void OnExit(core::AppContext& ctx) override;
    std::optional<core::GameState> HandleEvent(const core::InputEvent& event,
                                               core::AppContext& ctx) override;
    std::optional<core::GameState> Update(core::AppContext& ctx, double dt_ms) override;
    void Render(core::AppContext& ctx) override;
};

}  // namespace screens
