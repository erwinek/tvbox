#include "app/App.h"

#include "core/Clock.h"
#include "io/KeyboardInput.h"
#include "screens/AttractScreen.h"
#include "screens/EndGameScreen.h"
#include "screens/MeasureScreen.h"
#include "screens/ModeSelectScreen.h"
#include "util/Logger.h"

#include <SDL.h>

#include <filesystem>
#include <memory>

namespace app {

namespace {
// Mapowanie klawiszy symulacji UART (1..4) na stany.
core::GameState StateFromDebug(int value) {
    switch (value) {
        case 2:
            return core::GameState::ModeSelect;
        case 3:
            return core::GameState::Measure;
        case 4:
            return core::GameState::EndGame;
        case 1:
        default:
            return core::GameState::Attract;
    }
}
}  // namespace

App::App(const config::Config& cfg) : cfg_(cfg) {}

App::~App() {
    serial_.Stop();
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    store_.Close();
    audio_.Shutdown();
    renderer_.Shutdown();
}

bool App::Init() {
    std::filesystem::create_directories("data");
    std::filesystem::create_directories(cfg_.video_dir);

    if (!renderer_.Init(cfg_.window_width, cfg_.window_height, cfg_.fullscreen, cfg_.font_path,
                        cfg_.font_path_heading)) {
        return false;
    }

    if (!store_.Open(cfg_.db_path)) {
        util::Log(util::LogLevel::Error, "Failed to open leaderboard db");
        return false;
    }

    if (audio_.Init()) {
        audio_.LoadSound("coin", cfg_.sound_coin);
        audio_.LoadSound("hit", cfg_.sound_hit);
        audio_.LoadSound("select", cfg_.sound_select);
        audio_.LoadSound("win", cfg_.sound_win);
        audio_.LoadMusic(cfg_.music_attract);
    }

    video_.SetCommandTemplate(media::VideoCapture::ResolveCommandTemplate(cfg_.camera_command));
    sync_.Configure(cfg_.server_url, cfg_.auth_token);

    std::vector<game::GameMode> modes;
    for (const auto& m : cfg_.game_modes) {
        modes.push_back({m.id, m.name, m.multiplier});
    }
    session_.SetModes(std::move(modes));

    // Kontekst dla ekranow.
    ctx_.config = &cfg_;
    ctx_.renderer = &renderer_;
    ctx_.audio = &audio_;
    ctx_.video = &video_;
    ctx_.store = &store_;
    ctx_.session = &session_;
    ctx_.refresh_leaderboard = [this]() { RefreshLeaderboard(); };
    ctx_.commit_score = [this](const std::string& player, int score) {
        CommitScore(player, score);
    };

    RegisterScreens();

    if (!serial_.Start(cfg_.serial_port, cfg_.baud_rate, input_queue_)) {
        util::Log(util::LogLevel::Warn, "Serial reader not started");
    }

    RefreshLeaderboard();
    fsm_.Start(core::GameState::Attract, ctx_);
    return true;
}

void App::RegisterScreens() {
    fsm_.Register(core::GameState::Attract, std::make_unique<screens::AttractScreen>());
    fsm_.Register(core::GameState::ModeSelect, std::make_unique<screens::ModeSelectScreen>());
    fsm_.Register(core::GameState::Measure, std::make_unique<screens::MeasureScreen>());
    fsm_.Register(core::GameState::EndGame, std::make_unique<screens::EndGameScreen>());
}

void App::Run() {
    running_ = true;
    Uint32 last_ticks = SDL_GetTicks();

    while (running_) {
        // 1) Klawiatura / okno -> kolejka zdarzen.
        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event)) {
            if (auto event = io::TranslateKey(sdl_event)) {
                if (event->type == core::InputType::Quit) {
                    running_ = false;
                } else {
                    input_queue_.Push(*event);
                }
            }
        }

        // 2) Wszystkie zdarzenia (klawiatura + UART) -> maszyna stanow.
        while (auto event = input_queue_.Pop()) {
            if (event->type == core::InputType::Quit) {
                running_ = false;
            } else if (event->type == core::InputType::DebugGoto) {
                fsm_.GoTo(StateFromDebug(event->value), ctx_);
            } else {
                fsm_.HandleEvent(*event, ctx_);
            }
        }

        // 3) Update z delta-time.
        const Uint32 now_ticks = SDL_GetTicks();
        const double dt_ms = static_cast<double>(now_ticks - last_ticks);
        last_ticks = now_ticks;
        fsm_.Update(ctx_, dt_ms);

        // 4) Synchronizacja okresowa.
        const long long now = core::NowMs();
        if (cfg_.sync_enabled && (now - last_sync_ms_ > 10000)) {
            sync_.SyncOnce(store_, 5);
            last_sync_ms_ = now;
        }

        // 5) Odswiezenie rankingu po zakonczeniu nagrywania w tle.
        if (leaderboard_dirty_.exchange(false)) {
            RefreshLeaderboard();
        }

        // 6) Render aktywnego ekranu.
        fsm_.Render(ctx_);

        SDL_Delay(16);
    }
}

void App::CommitScore(const std::string& player_id, int score) {
    const long long ts = core::NowMs();
    util::Log(util::LogLevel::Info,
              "Commit score=" + std::to_string(score) + " player=" + player_id);

    std::string video_path;
    if (!cfg_.camera_command.empty()) {
        const std::string filename = "hit_" + std::to_string(ts) + ".mp4";
        video_path = cfg_.video_dir + "/" + filename;
        CaptureVideoAsync(video_path);
    }

    store::ScoreEntry entry{};
    entry.player_id = player_id;
    entry.score = score;
    entry.timestamp = ts;
    entry.video_path = video_path;
    entry.synced = 0;
    store_.AddScore(entry);

    RefreshLeaderboard();
}

void App::CaptureVideoAsync(const std::string& video_path) {
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    recording_ = true;
    capture_thread_ = std::thread([this, video_path]() {
        util::Log(util::LogLevel::Info, "Recording started: " + video_path);
        const bool ok = video_.CaptureClip(video_path, cfg_.camera_duration_ms);
        if (ok) {
            util::Log(util::LogLevel::Info, "Recording saved: " + video_path);
            const std::string thumb = media::VideoCapture::ThumbPathFor(video_path);
            media::VideoCapture::ExtractThumbnail(video_path, thumb);
            const std::string frames_dir = media::VideoCapture::FramesDirFor(video_path);
            media::VideoCapture::ExtractFrames(video_path, frames_dir, 10);
            leaderboard_dirty_ = true;  // odswiezenie wykona watek glowny
        } else {
            util::Log(util::LogLevel::Warn, "Recording failed: " + video_path);
        }
        recording_ = false;
    });
}

void App::RefreshLeaderboard() {
    auto top = store_.GetTopScores(cfg_.leaderboard_size);
    std::vector<ui::ScoreEntry> entries;
    for (const auto& e : top) {
        ui::ScoreEntry ue{};
        ue.player_id = e.player_id;
        ue.score = e.score;
        ue.timestamp = e.timestamp;
        ue.video_path = e.video_path;
        if (!e.video_path.empty()) {
            ue.thumb_path = media::VideoCapture::ThumbPathFor(e.video_path);
            if (!std::filesystem::exists(ue.thumb_path)) {
                ue.thumb_path.clear();
            }
            ue.frames_dir = media::VideoCapture::FramesDirFor(e.video_path);
            if (!std::filesystem::is_directory(ue.frames_dir)) {
                ue.frames_dir.clear();
            }
        }
        entries.push_back(ue);
    }
    ctx_.leaderboard = std::move(entries);
}

}  // namespace app
