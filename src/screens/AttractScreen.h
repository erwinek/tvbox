#pragma once

#include "core/Screen.h"
#include "ui/widgets/LeaderboardWidget.h"
#include "ui/widgets/ScrollBar.h"

namespace screens {

// CHOINKA: ekran bezczynnosci. Animacje, zachety do gry, ranking, dzwiek.
class AttractScreen : public core::Screen {
public:
    void OnEnter(core::AppContext& ctx) override;
    std::optional<core::GameState> HandleEvent(const core::InputEvent& event,
                                               core::AppContext& ctx) override;
    std::optional<core::GameState> Update(core::AppContext& ctx, double dt_ms) override;
    void Render(core::AppContext& ctx) override;

private:
    ui::widgets::LeaderboardWidget board_;
    ui::widgets::ScrollBar scroll_;

    // Serwis: potwierdzenie kasowania wszystkich danych (klawisz V, Enter = tak).
    bool purge_confirm_ = false;
    double purge_confirm_ms_ = 0.0;
    static constexpr double kPurgeConfirmTimeoutMs = 10000.0;
};

}  // namespace screens
