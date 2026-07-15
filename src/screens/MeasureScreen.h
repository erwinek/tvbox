#pragma once

#include "core/Screen.h"

namespace screens {

// MEASURE: oczekiwanie na uderzenie, a po nim naliczanie wyniku z efektem liczenia.
class MeasureScreen : public core::Screen {
public:
    void OnEnter(core::AppContext& ctx) override;
    void OnExit(core::AppContext& ctx) override;
    std::optional<core::GameState> HandleEvent(const core::InputEvent& event,
                                               core::AppContext& ctx) override;
    std::optional<core::GameState> Update(core::AppContext& ctx, double dt_ms) override;
    void Render(core::AppContext& ctx) override;

private:
    bool counting_ = false;
    int target_score_ = 0;
    double display_ = 0.0;
    double elapsed_ms_ = 0.0;
    double idle_ms_ = 0.0;

    static constexpr double kCountMs = 1500.0;
    static constexpr double kHoldMs = 800.0;
    static constexpr double kIdleTimeoutMs = 25000.0;
};

}  // namespace screens
