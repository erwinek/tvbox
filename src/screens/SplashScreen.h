#pragma once

#include "core/Screen.h"

namespace screens {

// Krotki boot splash: logo, animowane pierscienie, numer wersji.
class SplashScreen : public core::Screen {
public:
    void OnEnter(core::AppContext& ctx) override;
    std::optional<core::GameState> HandleEvent(const core::InputEvent& event,
                                               core::AppContext& ctx) override;
    std::optional<core::GameState> Update(core::AppContext& ctx, double dt_ms) override;
    void Render(core::AppContext& ctx) override;

private:
    double elapsed_ms_ = 0;
    bool skip_ = false;
};

}  // namespace screens
