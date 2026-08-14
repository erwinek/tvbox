#pragma once

#include "core/Screen.h"

#include <array>
#include <cstdint>

namespace screens {

// MEASURE: czekamy na uderzenie; SCORE z PGM steruje naliczaniem 1:1 z matryca.
class MeasureScreen : public core::Screen {
public:
    void OnEnter(core::AppContext& ctx) override;
    void OnExit(core::AppContext& ctx) override;
    std::optional<core::GameState> HandleEvent(const core::InputEvent& event,
                                               core::AppContext& ctx) override;
    std::optional<core::GameState> Update(core::AppContext& ctx, double dt_ms) override;
    void Render(core::AppContext& ctx) override;

private:
    struct Particle {
        float x = 0;
        float y = 0;
        float vx = 0;
        float vy = 0;
        float life = 0;   // 0..1
        float size = 4;
        std::uint8_t r = 255;
        std::uint8_t g = 200;
        std::uint8_t b = 80;
    };

    struct Shockwave {
        float radius = 0;
        float life = 0;  // 0..1 remaining
        float thickness = 8;
    };

    void ResetFx();
    void SpawnBurst(int cx, int cy, int count, float speed_scale);
    void SpawnShockwave();
    void UpdateFx(double dt_ms, int cx, int cy);

    bool counting_ = false;
    bool committed_ = false;
    bool impact_seen_ = false;
    bool got_pgm_score_ = false;
    int display_score_ = 0;
    int target_score_ = 0;
    int last_fx_score_ = -1;
    double quiet_ms_ = 0.0;
    double hold_ms_ = 0.0;
    double idle_ms_ = 0.0;
    double fx_ms_ = 0.0;
    double score_tick_ms_ = 0.0;

    static constexpr int kMaxParticles = 96;
    static constexpr int kMaxWaves = 6;
    std::array<Particle, kMaxParticles> particles_{};
    std::array<Shockwave, kMaxWaves> waves_{};
    int particle_cursor_ = 0;
    int wave_cursor_ = 0;

    // Po dogonieniu targetu krotki hold → EndGame.
    static constexpr double kQuietMs = 220.0;
    static constexpr double kHoldMs = 400.0;
    static constexpr double kScoreTickMs = 22.0;
};

}  // namespace screens
