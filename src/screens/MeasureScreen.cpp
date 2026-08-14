#include "screens/MeasureScreen.h"

#include "core/AppContext.h"
#include "game/GameSession.h"
#include "game/ScoreEngine.h"
#include "media/AudioPlayer.h"
#include "ui/BackgroundPlayer.h"
#include "ui/Renderer.h"
#include "ui/widgets/Header.h"
#include "ui/widgets/Hud.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace screens {

namespace {

constexpr float kPi = 3.14159265f;

float Frand() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

}  // namespace

void MeasureScreen::ResetFx() {
    fx_ms_ = 0.0;
    last_fx_score_ = -1;
    particle_cursor_ = 0;
    wave_cursor_ = 0;
    for (auto& p : particles_) {
        p.life = 0;
    }
    for (auto& w : waves_) {
        w.life = 0;
    }
}

void MeasureScreen::SpawnBurst(int cx, int cy, int count, float speed_scale) {
    for (int i = 0; i < count; ++i) {
        Particle& p = particles_[static_cast<std::size_t>(particle_cursor_++ % kMaxParticles)];
        const float ang = Frand() * kPi * 2.0f;
        const float spd = (120.0f + Frand() * 420.0f) * speed_scale;
        p.x = static_cast<float>(cx) + (Frand() - 0.5f) * 40.0f;
        p.y = static_cast<float>(cy) + (Frand() - 0.5f) * 40.0f;
        p.vx = std::cos(ang) * spd;
        p.vy = std::sin(ang) * spd;
        p.life = 0.75f + Frand() * 0.55f;
        p.size = 3.0f + Frand() * 10.0f;
        const int tint = std::rand() % 3;
        if (tint == 0) {
            p.r = 255;
            p.g = static_cast<std::uint8_t>(180 + std::rand() % 75);
            p.b = 40;
        } else if (tint == 1) {
            p.r = 255;
            p.g = static_cast<std::uint8_t>(80 + std::rand() % 80);
            p.b = 60;
        } else {
            p.r = 255;
            p.g = 255;
            p.b = static_cast<std::uint8_t>(200 + std::rand() % 55);
        }
    }
}

void MeasureScreen::SpawnShockwave() {
    Shockwave& w = waves_[static_cast<std::size_t>(wave_cursor_++ % kMaxWaves)];
    w.radius = 20.0f;
    w.life = 1.0f;
    w.thickness = 10.0f + Frand() * 10.0f;
}

void MeasureScreen::UpdateFx(double dt_ms, int cx, int cy) {
    const float dt = static_cast<float>(dt_ms) / 1000.0f;
    fx_ms_ += dt_ms;

    for (auto& p : particles_) {
        if (p.life <= 0.0f) {
            continue;
        }
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vx *= 0.985f;
        p.vy = p.vy * 0.985f + 280.0f * dt;  // lekka grawitacja
        p.life -= dt * 0.85f;
    }

    for (auto& w : waves_) {
        if (w.life <= 0.0f) {
            continue;
        }
        w.radius += 780.0f * dt;
        w.life -= dt * 1.15f;
        w.thickness = std::max(2.0f, w.thickness - 8.0f * dt);
    }

    // Ciągłe iskry podczas naliczania.
    if (counting_ && quiet_ms_ < kQuietMs && (static_cast<int>(fx_ms_) / 40) !=
                                                 (static_cast<int>(fx_ms_ - dt_ms) / 40)) {
        SpawnBurst(cx, cy, 3, 0.45f);
    }
}

void MeasureScreen::OnEnter(core::AppContext& ctx) {
    counting_ = false;
    committed_ = false;
    impact_seen_ = false;
    got_pgm_score_ = false;
    display_score_ = 0;
    target_score_ = 0;
    quiet_ms_ = 0.0;
    hold_ms_ = 0.0;
    idle_ms_ = 0.0;
    ResetFx();
    score_tick_ms_ = 0.0;

    // PGM czesto pomija ModeSelect (START -> Measure) — petla musi grac tu, przed ciosem.
    if (ctx.background) {
        ctx.background->SetPlaying(true);
    }

    if (ctx.start_measure_recording) {
        ctx.start_measure_recording();
    }
}

void MeasureScreen::OnExit(core::AppContext& ctx) {
    if (ctx.background) {
        ctx.background->SetPlaying(false);
    }
    if (ctx.cancel_measure_recording) {
        ctx.cancel_measure_recording();
    }
}

std::optional<core::GameState> MeasureScreen::HandleEvent(const core::InputEvent& event,
                                                          core::AppContext& ctx) {
    if (event.type != core::InputType::Hit) {
        return std::nullopt;
    }

    // HIT z PGM: od razu UI naliczania (nie czekaj na SCORE — UART bywa gubiony).
    if (event.text == "impact") {
        impact_seen_ = true;
        if (ctx.background) {
            ctx.background->SetPlaying(false);
        }
        if (ctx.freeze_measure_recording) {
            ctx.freeze_measure_recording();
        }
        const ui::Layout& lay = ctx.renderer->layout();
        if (!counting_) {
            counting_ = true;
            display_score_ = 0;
            target_score_ = 0;
            quiet_ms_ = 0.0;
            hold_ms_ = 0.0;
            score_tick_ms_ = 0.0;
            fx_ms_ = 0.0;
        }
        SpawnBurst(lay.CenterX(), lay.CenterY(), 24, 1.0f);
        SpawnShockwave();
        if (ctx.audio) {
            ctx.audio->PlaySound("hit");
        }
        return std::nullopt;
    }

    // Wartosc z PGM Naliczanie — bez mnoznika (PGM juz ma finalna skale).
    int value = event.value;
    if (value <= 0) {
        // Symulacja (SPACE / STATE,HIT): brak wyniku z PGM — licz lokalnie.
        got_pgm_score_ = false;
        if (!counting_) {
            counting_ = true;
            display_score_ = 0;
            target_score_ = 0;
            score_tick_ms_ = 0.0;
            if (ctx.background) {
                ctx.background->SetPlaying(false);
            }
            if (!impact_seen_ && ctx.freeze_measure_recording) {
                ctx.freeze_measure_recording();
            }
            const ui::Layout& lay = ctx.renderer->layout();
            SpawnBurst(lay.CenterX(), lay.CenterY(), 48, 1.15f);
            SpawnShockwave();
            SpawnShockwave();
            if (!impact_seen_ && ctx.audio) {
                ctx.audio->PlaySound("hit");
            }
        }
        quiet_ms_ = 0.0;
        hold_ms_ = 0.0;
        return std::nullopt;
    }

    got_pgm_score_ = true;

    const ui::Layout& lay = ctx.renderer->layout();
    const int cx = lay.CenterX();
    const int cy = lay.CenterY();

    if (!counting_) {
        counting_ = true;
        display_score_ = 0;
        if (ctx.background) {
            ctx.background->SetPlaying(false);
        }
        quiet_ms_ = 0.0;
        hold_ms_ = 0.0;
        score_tick_ms_ = 0.0;
        if (!impact_seen_) {
            fx_ms_ = 0.0;
        }
        if (!impact_seen_ && ctx.freeze_measure_recording) {
            ctx.freeze_measure_recording();
        }
        SpawnBurst(cx, cy, 48, 1.15f);
        SpawnShockwave();
        SpawnShockwave();
        if (!impact_seen_ && ctx.audio) {
            ctx.audio->PlaySound("hit");
        }
    } else if (value > last_fx_score_ + 8) {
        SpawnBurst(cx, cy, 8, 0.7f);
        if ((value / 80) != (last_fx_score_ / 80)) {
            SpawnShockwave();
        }
    }
    last_fx_score_ = value;

    if (value > target_score_) {
        target_score_ = value;
    }
    ctx.session->SetScore(target_score_);
    quiet_ms_ = 0.0;
    hold_ms_ = 0.0;
    return std::nullopt;
}

std::optional<core::GameState> MeasureScreen::Update(core::AppContext& ctx, double dt_ms) {
    const ui::Layout& lay = ctx.renderer->layout();
    if (counting_) {
        UpdateFx(dt_ms, lay.CenterX(), lay.CenterY());
        score_tick_ms_ += dt_ms;
        while (score_tick_ms_ >= kScoreTickMs && display_score_ < target_score_) {
            score_tick_ms_ -= kScoreTickMs;
            if (display_score_ + 33 < target_score_) {
                display_score_ += 13;
            } else {
                display_score_ += 1;
            }
            if (display_score_ > target_score_) {
                display_score_ = target_score_;
            }
        }
        ctx.session->SetScore(std::max(display_score_, target_score_));
    }

    if (!counting_) {
        idle_ms_ += dt_ms;
        (void)idle_ms_;
        return std::nullopt;
    }

    // Czekaj na wynik z PGM — bez SCORE nie konczymy (nie commituj 0).
    if (!got_pgm_score_) {
        quiet_ms_ = 0.0;
        hold_ms_ = 0.0;
        return std::nullopt;
    }

    if (display_score_ < target_score_) {
        quiet_ms_ = 0.0;
        hold_ms_ = 0.0;
        return std::nullopt;
    }

    quiet_ms_ += dt_ms;
    if (quiet_ms_ < kQuietMs) {
        return std::nullopt;
    }

    hold_ms_ += dt_ms;
    if (!committed_) {
        committed_ = true;
        display_score_ = target_score_;
        ctx.session->SetScore(target_score_);
        const std::string player = ctx.session->player_id();
        if (ctx.commit_score) {
            ctx.commit_score(player, target_score_);
        }
        SpawnBurst(lay.CenterX(), lay.CenterY(), 36, 1.0f);
        SpawnShockwave();
    }

    if (hold_ms_ >= kHoldMs) {
        return core::GameState::EndGame;
    }
    return std::nullopt;
}

void MeasureScreen::Render(core::AppContext& ctx) {
    ui::Renderer& r = *ctx.renderer;
    const ui::Layout& lay = r.layout();
    const Uint32 elapsed = r.ticks();
    const int cx = lay.CenterX();
    const int cy = lay.CenterY();
    const int w = r.width();
    const int h = r.height();

    r.BeginFrame(SDL_Color{0, 0, 0, 255});

    if (!counting_) {
        r.DrawVerticalGradient(SDL_Color{44, 18, 30, 255}, SDL_Color{8, 6, 16, 255});
    } else {
        // Dynamiczne tlo: cieplejsze wraz z wynikiem + puls.
        const float t = std::min(1.0f, static_cast<float>(display_score_) / 900.0f);
        const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(fx_ms_) * 0.012f);
        const Uint8 top_r = static_cast<Uint8>(50 + t * 90 + pulse * 25);
        const Uint8 top_g = static_cast<Uint8>(12 + t * 30);
        const Uint8 top_b = static_cast<Uint8>(28 + (1.0f - t) * 40);
        r.DrawVerticalGradient(SDL_Color{top_r, top_g, top_b, 255}, SDL_Color{10, 4, 14, 255});

        // Radialne "speed lines".
        const int lines = 18;
        for (int i = 0; i < lines; ++i) {
            const float ang = (static_cast<float>(i) / lines) * kPi * 2.0f +
                              static_cast<float>(fx_ms_) * 0.0015f;
            const float spin = 0.55f + 0.45f * std::sin(static_cast<float>(fx_ms_) * 0.008f + i);
            const int len = static_cast<int>(lay.PH(0.22f) + spin * lay.PH(0.28f));
            const int x0 = cx + static_cast<int>(std::cos(ang) * lay.PM(0.04f));
            const int y0 = cy + static_cast<int>(std::sin(ang) * lay.PM(0.04f));
            const int x1 = cx + static_cast<int>(std::cos(ang) * len);
            const int y1 = cy + static_cast<int>(std::sin(ang) * len);
            const int steps = 12;
            for (int s = 0; s < steps; ++s) {
                const float u = static_cast<float>(s) / steps;
                const int px = x0 + static_cast<int>((x1 - x0) * u);
                const int py = y0 + static_cast<int>((y1 - y0) * u);
                const Uint8 a = static_cast<Uint8>((1.0f - u) * 55 * spin);
                const int sz = 2 + static_cast<int>((1.0f - u) * 5);
                r.FillRect(SDL_Rect{px, py, sz, sz}, SDL_Color{255, 160, 60, a});
            }
        }

        // Expanding shockwaves (przyblizenie pierscieni kwadratami/segmentami).
        for (const auto& wave : waves_) {
            if (wave.life <= 0.0f) {
                continue;
            }
            const Uint8 a = static_cast<Uint8>(wave.life * 160);
            const int rad = static_cast<int>(wave.radius);
            const int thick = std::max(2, static_cast<int>(wave.thickness));
            const int segs = 36;
            for (int i = 0; i < segs; ++i) {
                const float ang = (static_cast<float>(i) / segs) * kPi * 2.0f;
                const int px = cx + static_cast<int>(std::cos(ang) * rad) - thick / 2;
                const int py = cy + static_cast<int>(std::sin(ang) * rad) - thick / 2;
                r.FillRect(SDL_Rect{px, py, thick, thick}, SDL_Color{255, 220, 100, a});
            }
        }

        // Particles.
        for (const auto& p : particles_) {
            if (p.life <= 0.0f) {
                continue;
            }
            const Uint8 a = static_cast<Uint8>(std::min(1.0f, p.life) * 255);
            const int sz = std::max(2, static_cast<int>(p.size * p.life));
            r.FillRect(SDL_Rect{static_cast<int>(p.x) - sz / 2, static_cast<int>(p.y) - sz / 2, sz,
                                sz},
                       SDL_Color{p.r, p.g, p.b, a});
        }

        // Flash na starcie / finiszu.
        if (fx_ms_ < 180.0) {
            const float f = 1.0f - static_cast<float>(fx_ms_ / 180.0);
            r.FillRect(SDL_Rect{0, 0, w, h},
                       SDL_Color{255, 240, 200, static_cast<Uint8>(f * 70)});
        } else if (committed_ && hold_ms_ < 200.0) {
            const float f = 1.0f - static_cast<float>(hold_ms_ / 200.0);
            r.FillRect(SDL_Rect{0, 0, w, h},
                       SDL_Color{255, 215, 80, static_cast<Uint8>(f * 50)});
        }
    }

    const int header_h = ui::widgets::RenderHeader(r);

    const std::string mode = ctx.session->selected_mode().name;
    SDL_Point mw = r.MeasureText(mode, ui::FontSize::Normal);
    const int badge_pad_x = lay.PM(0.026f);
    const int badge_pad_y = lay.PM(0.019f);
    const int badge_y = header_h + lay.PH(0.03f);
    const SDL_Rect badge{cx - mw.x / 2 - badge_pad_x, badge_y, mw.x + 2 * badge_pad_x,
                         mw.y + badge_pad_y};
    r.Panel(badge, lay.PM(0.017f), SDL_Color{30, 34, 64, 130}, SDL_Color{90, 100, 170, 150});
    r.DrawText(mode, ui::FontSize::Normal, SDL_Color{170, 205, 255, 255}, cx,
               badge_y + badge_pad_y / 2, true);

    if (!counting_) {
        const std::string& mode_id = ctx.session->selected_mode().id;
        const char* prompt = "HIT NOW!";
        if (mode_id == "kopacz") {
            prompt = "KICK THE BALL!";
        } else if (mode_id == "hammer") {
            prompt = "USE THE HAMMER!";
        } else if (mode_id == "boxer") {
            prompt = "HIT THE PUNCH!";
        }
        float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(elapsed) * 0.006f);
        Uint8 alpha = static_cast<Uint8>(140 + static_cast<int>(pulse * 115));
        const int prompt_h = r.MeasureText(prompt, ui::FontSize::Large).y;
        const int gap = lay.PH(0.018f);
        const int prompt_y = badge_y + badge.h + gap;
        const int vid_h = lay.PH(0.42f);
        const int vid_w = (vid_h * 9) / 16;
        const int vid_y = prompt_y + prompt_h + gap;
        const SDL_Rect video{cx - vid_w / 2, vid_y, vid_w, vid_h};

        r.DrawText(prompt, ui::FontSize::Large, SDL_Color{255, 90, 90, 255}, cx, prompt_y, true,
                   alpha, 1.0f, 3);
        r.Panel(SDL_Rect{video.x - 4, video.y - 4, video.w + 8, video.h + 8}, lay.PM(0.01f),
                SDL_Color{0, 0, 0, 180}, SDL_Color{255, 255, 255, 40});
        if (!(ctx.background && ctx.background->Render(r, video))) {
            r.FillRect(video, SDL_Color{20, 20, 28, 255});
        }
    } else {
        const bool finishing = quiet_ms_ >= kQuietMs;
        float scale = finishing ? 1.85f : 1.5f;
        if (!finishing) {
            scale += 0.08f * std::sin(static_cast<float>(fx_ms_) * 0.02f);
            // Lekki "punch" przy kazdym skoku wyniku.
            scale += 0.04f * std::sin(static_cast<float>(display_score_) * 0.2f);
        }
        const std::string value = std::to_string(display_score_);
        // Cien / glow: drugi pass.
        r.DrawText(value, ui::FontSize::Huge, SDL_Color{255, 120, 40, 180}, cx + 3,
                   cy - lay.PH(0.11f) + 3, true, 160, scale * 1.02f, 0);
        r.DrawText(value, ui::FontSize::Huge, SDL_Color{255, 230, 80, 255}, cx,
                   cy - lay.PH(0.11f), true, 255, scale, 5);
    }

    ui::widgets::RenderHud(r, ctx.session->credits().count(), ctx.leaderboard);
    r.EndFrame();
}

}  // namespace screens
