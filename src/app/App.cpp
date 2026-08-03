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
                        cfg_.font_path_heading, cfg_.use_kms, cfg_.layout_scale,
                        cfg_.display_width, cfg_.display_height, cfg_.display_rotate)) {
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
    background_.Init(cfg_.background_dir);

    std::vector<game::GameMode> modes;
    for (const auto& m : cfg_.game_modes) {
        modes.push_back({m.id, m.name, m.multiplier});
    }
    session_.SetModes(std::move(modes));

    // Kontekst dla ekranow.
    ctx_.config = &cfg_;
    ctx_.renderer = &renderer_;
    ctx_.background = &background_;
    ctx_.audio = &audio_;
    ctx_.video = &video_;
    ctx_.store = &store_;
    ctx_.session = &session_;
    ctx_.refresh_leaderboard = [this]() { RefreshLeaderboard(); };
    ctx_.commit_score = [this](const std::string& player, int score) {
        CommitScore(player, score);
    };
    ctx_.start_measure_recording = [this]() { StartMeasureRecording(); };
    ctx_.cancel_measure_recording = [this]() { CancelMeasureRecording(); };
    ctx_.freeze_measure_recording = [this]() { FreezeMeasureRecording(); };

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
            } else if (event->type == core::InputType::SyncState) {
                ApplySyncState(*event);
            } else {
                if (event->type == core::InputType::Start && !event->text.empty()) {
                    last_pgm_phase_ = "measure";
                }
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

        // 5) Finalizacja nagrania po uderzeniu (bufor RAM + 1s opoznienia).
        PollRecordingFinalize();

        // 6) Odswiezenie rankingu po zakonczeniu nagrywania w tle.
        if (leaderboard_dirty_.exchange(false)) {
            RefreshLeaderboard();
        }

        // 7) Render aktywnego ekranu.
        const Uint32 frame_start = SDL_GetTicks();
        fsm_.Render(ctx_);

        // FPS log co 120 klatek — do diagnozy płynnosci na Wyse.
        static int fps_frames = 0;
        static Uint32 fps_start = 0;
        if (fps_start == 0) fps_start = frame_start;
        ++fps_frames;
        if (fps_frames >= 120) {
            const Uint32 elapsed = frame_start - fps_start;
            util::Log(util::LogLevel::Info,
                      "FPS: " + std::to_string(elapsed > 0 ? fps_frames * 1000 / elapsed : 0));
            fps_frames = 0;
            fps_start = frame_start;
        }

        // Frame pacing: ~50fps na Wyse (Celeron 4K); vsync i tak moze obcinac.
        const Uint32 frame_ms = SDL_GetTicks() - frame_start;
        if (frame_ms < 20) {
            SDL_Delay(20 - frame_ms);
        }
    }
}

void App::ApplySyncState(const core::InputEvent& event) {
    // text = "phase:mode" (np. measure:boxer), value = credit
    std::string phase = event.text;
    std::string mode;
    const auto colon = event.text.find(':');
    if (colon != std::string::npos) {
        phase = event.text.substr(0, colon);
        mode = event.text.substr(colon + 1);
    }
    session_.credits().Set(event.value);

    if (phase == last_pgm_phase_) {
        return;  // ta sama faza — tylko kredyt (powyzej)
    }
    last_pgm_phase_ = phase;

    if (phase == "measure") {
        if (!mode.empty()) {
            session_.BeginRoundFromPgm(mode);
        }
        // Nie cofaj EndGame → Measure gdy PGM jeszcze konczy POMIAR (celebration):
        // last_pgm_phase_ juz "measure" po START, wiec tu wchodzimy tylko przy zmianie fazy.
        fsm_.GoTo(core::GameState::Measure, ctx_);
        return;
    }
    if (phase == "gamestart") {
        fsm_.GoTo(core::GameState::ModeSelect, ctx_);
        return;
    }
    // attract (i nieznane)
    fsm_.GoTo(core::GameState::Attract, ctx_);
}

void App::CommitScore(const std::string& player_id, int score) {
    const long long ts = core::NowMs();
    util::Log(util::LogLevel::Info,
              "Commit score=" + std::to_string(score) + " player=" + player_id);

    // Nagranie zamykane w momencie HIT — tu tylko przypisz sciezke do rankingu.
    if (committed_video_path_.empty() && !cfg_.camera_command.empty()) {
        FreezeMeasureRecording();
    }
    const std::string video_path = committed_video_path_;

    store::ScoreEntry entry{};
    entry.player_id = player_id;
    entry.score = score;
    entry.timestamp = ts;
    entry.video_path = video_path;
    entry.synced = 0;
    store_.AddScore(entry);

    RefreshLeaderboard();
}

void App::StartMeasureRecording() {
    if (cfg_.camera_command.empty()) {
        return;
    }
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    hit_finalize_scheduled_ = false;
    finalize_at_ms_ = 0;
    pending_video_path_.clear();
    committed_video_path_.clear();

    const bool ok = video_.StartRingBufferCapture(cfg_.camera_duration_ms);
    recording_ = ok;
    if (ok) {
        util::Log(util::LogLevel::Info, "Measure recording buffer started");
    }
}

void App::FreezeMeasureRecording() {
    if (cfg_.camera_command.empty()) {
        return;
    }
    if (hit_finalize_scheduled_) {
        return;
    }
    if (!video_.IsRingBufferActive()) {
        return;
    }
    const long long ts = core::NowMs();
    const std::string filename = "hit_" + std::to_string(ts) + ".mp4";
    const std::string video_path = cfg_.video_dir + "/" + filename;
    ScheduleHitRecordingFinalize(video_path);
}

void App::ScheduleHitRecordingFinalize(const std::string& video_path) {
    pending_video_path_ = video_path;
    committed_video_path_ = video_path;
    // Domyslnie 0 — stop zaraz po uderzeniu (HIT), bez dogrywania post-hit.
    finalize_at_ms_ = core::NowMs() + cfg_.camera_post_hit_ms;
    hit_finalize_scheduled_ = true;
    util::Log(util::LogLevel::Info,
              "Hit recording finalize scheduled in " + std::to_string(cfg_.camera_post_hit_ms) + "ms");
}

void App::CancelMeasureRecording() {
    if (hit_finalize_scheduled_) {
        return;
    }
    video_.StopRingBufferCapture();
    recording_ = false;
    finalize_at_ms_ = 0;
    pending_video_path_.clear();
    util::Log(util::LogLevel::Info, "Measure recording cancelled");
}

void App::PollRecordingFinalize() {
    if (finalize_at_ms_ == 0) {
        return;
    }
    if (core::NowMs() < finalize_at_ms_) {
        return;
    }

    finalize_at_ms_ = 0;
    const std::string video_path = pending_video_path_;
    pending_video_path_.clear();

    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }

    capture_thread_ = std::thread([this, video_path]() {
        recording_ = true;
        util::Log(util::LogLevel::Info, "Finalizing hit recording: " + video_path);
        const bool ok = video_.FinalizeRingBuffer(video_path);
        if (ok) {
            util::Log(util::LogLevel::Info, "Recording saved: " + video_path);
            const std::string thumb = media::VideoCapture::ThumbPathFor(video_path);
            media::VideoCapture::ExtractThumbnail(video_path, thumb);
            const std::string frames_dir = media::VideoCapture::FramesDirFor(video_path);
            media::VideoCapture::ExtractFrames(video_path, frames_dir, 10);
            leaderboard_dirty_ = true;
        } else {
            util::Log(util::LogLevel::Warn, "Recording failed: " + video_path);
        }
        recording_ = false;
        hit_finalize_scheduled_ = false;
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
