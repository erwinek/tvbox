#include "screens/ModeSelectScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "media/AudioPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"

namespace screens {

void ModeSelectScreen::OnEnter(core::AppContext& ctx) {
    (void)ctx;
    idle_ms_ = 0.0;
}

std::optional<core::GameState> ModeSelectScreen::HandleEvent(const core::InputEvent& event,
                                                             core::AppContext& ctx) {
    idle_ms_ = 0.0;
    switch (event.type) {
        case core::InputType::Coin:
            ctx.session->credits().Add();
            if (ctx.audio) {
                ctx.audio->PlaySound("coin");
            }
            return std::nullopt;
        case core::InputType::SelectMode:
            ctx.session->MoveSelection(event.value);
            if (ctx.audio) {
                ctx.audio->PlaySound("select");
            }
            return std::nullopt;
        case core::InputType::Confirm:
            if (ctx.session->StartGame()) {
                return core::GameState::Measure;
            }
            return std::nullopt;
        case core::InputType::Back:
            return core::GameState::Attract;
        default:
            return std::nullopt;
    }
}

std::optional<core::GameState> ModeSelectScreen::Update(core::AppContext& ctx, double dt_ms) {
    (void)ctx;
    idle_ms_ += dt_ms;
    if (idle_ms_ >= kIdleTimeoutMs) {
        return core::GameState::Attract;
    }
    return std::nullopt;
}

void ModeSelectScreen::Render(core::AppContext& ctx) {
    ui::Renderer& r = *ctx.renderer;
    const int w = r.width();

    r.BeginFrame(SDL_Color{8, 8, 18, 255});
    ui::widgets::RenderHeader(r);

    r.DrawText("WYBIERZ TRYB", ui::FontSize::Large, SDL_Color{255, 215, 0, 255}, w / 2, 180, true);

    const auto& modes = ctx.session->modes();
    const int selected = ctx.session->selected_index();
    int y = 340;
    for (int i = 0; i < static_cast<int>(modes.size()); ++i) {
        const bool active = (i == selected);
        SDL_Color color = active ? SDL_Color{100, 255, 140, 255} : SDL_Color{160, 160, 180, 255};
        const std::string label = (active ? "> " : "  ") + modes[i].name + (active ? " <" : "");
        r.DrawText(label, ui::FontSize::Large, color, w / 2, y, true);
        y += 110;
    }

    r.DrawText("strzalki = wybor    ENTER = start", ui::FontSize::Small,
               SDL_Color{150, 150, 170, 255}, w / 2, r.height() - 80, true);

    r.EndFrame();
}

}  // namespace screens
