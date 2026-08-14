#include "screens/SplashScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "version.h"

#include <cmath>

namespace screens {

namespace {
constexpr double kSplashMs = 2800.0;
constexpr int kRingCount = 5;
constexpr int kSparkCount = 48;
}  // namespace

void SplashScreen::OnEnter(core::AppContext& ctx) {
    (void)ctx;
    elapsed_ms_ = 0;
    skip_ = false;
}

std::optional<core::GameState> SplashScreen::HandleEvent(const core::InputEvent& event,
                                                         core::AppContext& ctx) {
    switch (event.type) {
        case core::InputType::Quit:
            return std::nullopt;
        case core::InputType::Coin: {
            if (event.text == "abs") {
                ctx.session->credits().Set(event.value);
            } else {
                ctx.session->credits().Add(event.value > 0 ? event.value : 1);
            }
            skip_ = true;
            return std::nullopt;
        }
        case core::InputType::Start:
            skip_ = true;
            return std::nullopt;
        default:
            return std::nullopt;
    }
}

std::optional<core::GameState> SplashScreen::Update(core::AppContext& ctx, double dt_ms) {
    (void)ctx;
    elapsed_ms_ += dt_ms;
    if (skip_ || elapsed_ms_ >= kSplashMs) {
        if (ctx.session->credits().Has()) {
            return core::GameState::ModeSelect;
        }
        return core::GameState::Attract;
    }
    return std::nullopt;
}

void SplashScreen::Render(core::AppContext& ctx) {
    ui::Renderer& r = *ctx.renderer;
    const ui::Layout& lay = r.layout();
    const int w = r.width();
    const int h = r.height();
    const int cx = lay.CenterX();
    const int cy = static_cast<int>(h * 0.42f);
    const float t = static_cast<float>(elapsed_ms_);
    const float fade_in = std::min(1.f, t / 280.f);
    const float fade_out =
        elapsed_ms_ > kSplashMs - 350.0 ? static_cast<float>((kSplashMs - elapsed_ms_) / 350.0) : 1.f;
    const float vis = std::max(0.f, fade_in * fade_out);
    const Uint8 va = static_cast<Uint8>(vis * 255);

    r.BeginFrame(SDL_Color{0, 0, 0, 255});
    r.DrawVerticalGradient(SDL_Color{12, 14, 36, 255}, SDL_Color{4, 5, 12, 255});

    const float max_r = static_cast<float>(std::min(w, h)) * 0.42f;
    for (int ring = 0; ring < kRingCount; ++ring) {
        const float phase = t * 0.22f + static_cast<float>(ring) * (max_r / kRingCount);
        const float radius = std::fmod(phase, max_r);
        const float glow = 1.f - radius / max_r;
        const Uint8 a = static_cast<Uint8>(vis * glow * 90);
        const int dots = 28 + ring * 4;
        const int size = std::max(2, lay.PM(0.004f + glow * 0.003f));
        SDL_Color accent = ui::widgets::AccentColor(static_cast<Uint32>(t) + ring * 80);
        accent.a = a;
        for (int i = 0; i < dots; ++i) {
            const float ang = (static_cast<float>(i) / dots) * 6.283185f + t * 0.0004f;
            const int x = cx + static_cast<int>(std::cos(ang) * radius);
            const int y = cy + static_cast<int>(std::sin(ang) * radius * 0.92f);
            r.FillRect(SDL_Rect{x - size / 2, y - size / 2, size, size}, accent);
        }
    }

    for (int i = 0; i < kSparkCount; ++i) {
        const float seed = static_cast<float>(i * 97 + 13);
        const float px = std::fmod(seed * 9.1f + t * (0.04f + seed * 0.00008f), static_cast<float>(w));
        const float py = std::fmod(seed * 5.7f + t * 0.03f, static_cast<float>(h));
        const float br = 0.4f + 0.6f * std::sin(t * 0.006f + seed);
        const Uint8 a = static_cast<Uint8>(vis * (20 + br * 70));
        const int sz = 1 + static_cast<int>(br * 3);
        r.FillRect(SDL_Rect{static_cast<int>(px), static_cast<int>(py), sz, sz},
                   SDL_Color{140, 180, 255, a});
    }

    const float pulse = 0.92f + 0.08f * std::sin(t * 0.007f);
    r.DrawText("TVBOX", ui::FontSize::Huge, SDL_Color{255, 220, 80, va}, cx, cy - lay.PH(0.07f),
               true, va, pulse, 4);
    r.DrawText(tvbox::VersionString(), ui::FontSize::Large, SDL_Color{180, 210, 255, va}, cx,
               cy + lay.PH(0.03f), true, va, 1.0f, 2);
    r.DrawText(tvbox::VersionDetail(), ui::FontSize::Small, SDL_Color{140, 150, 180, va}, cx,
               cy + lay.PH(0.08f), true, va, 1.0f, 1);
    r.DrawText("POWERING UP", ui::FontSize::Normal, SDL_Color{90, 200, 255, va}, cx,
               h - lay.PH(0.12f), true, va, 1.0f, 1);

    r.EndFrame();
}

}  // namespace screens
