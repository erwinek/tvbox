#pragma once

#include "core/Screen.h"
#include "ui/FrameSequencePlayer.h"
#include "ui/widgets/LeaderboardWidget.h"
#include "ui/widgets/ScrollBar.h"

#include <string>

namespace screens {

// END_GAME: gratulacje, wynik, powtorka uderzenia i zachety do dalszej gry.
class EndGameScreen : public core::Screen {
public:
    void OnEnter(core::AppContext& ctx) override;
    std::optional<core::GameState> HandleEvent(const core::InputEvent& event,
                                               core::AppContext& ctx) override;
    std::optional<core::GameState> Update(core::AppContext& ctx, double dt_ms) override;
    void Render(core::AppContext& ctx) override;

private:
    double elapsed_ms_ = 0.0;
    std::string clip_dir_;
    ui::FrameSequencePlayer replay_;
    ui::widgets::LeaderboardWidget board_;
    ui::widgets::ScrollBar scroll_;

    static constexpr double kTimeoutMs = 9000.0;
};

}  // namespace screens
