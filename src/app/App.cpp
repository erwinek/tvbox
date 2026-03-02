#include "app/App.h"

#include "util/Logger.h"

#include <SDL.h>

#include <chrono>
#include <filesystem>
#include <sstream>

namespace app {

static long long NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

static std::vector<std::string> SplitCsv(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) {
        parts.push_back(item);
    }
    return parts;
}

App::App(const config::Config& cfg) : cfg_(cfg) {}

App::~App() {
    serial_.Stop();
    store_.Close();
    ui_.Shutdown();
}

bool App::Init() {
    std::filesystem::create_directories("data");
    std::filesystem::create_directories(cfg_.video_dir);

    ui_.SetFontPath(cfg_.font_path);
    if (!ui_.Init(cfg_.window_width, cfg_.window_height, cfg_.fullscreen)) {
        return false;
    }

    if (!store_.Open(cfg_.db_path)) {
        util::Log(util::LogLevel::Error, "Failed to open leaderboard db");
        return false;
    }

    video_.SetCommandTemplate(cfg_.camera_command);
    sync_.Configure(cfg_.server_url, cfg_.auth_token);

    if (!serial_.Start(cfg_.serial_port, cfg_.baud_rate, [this](const std::string& line) {
        HandleLine(line);
    })) {
        util::Log(util::LogLevel::Warn, "Serial reader not started");
    }

    RefreshLeaderboard();
    return true;
}

void App::Run() {
    running_ = true;
    SDL_Event event;
    const int frame_ms = 16;

    while (running_) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running_ = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running_ = false;
            }
        }

        const long long now = NowMs();
        if (cfg_.sync_enabled && (now - last_sync_ms_ > 10000)) {
            sync_.SyncOnce(store_, 5);
            last_sync_ms_ = now;
        }

        ui_.Render();
        SDL_Delay(frame_ms);
    }
}

void App::HandleLine(const std::string& line) {
    if (line.empty()) {
        return;
    }
    util::Log(util::LogLevel::Info, "Serial: " + line);
    auto parts = SplitCsv(line);
    if (parts.empty()) {
        return;
    }
    if (parts[0] == "SCORE" && parts.size() >= 4) {
        const int score = std::stoi(parts[1]);
        const std::string player_id = parts[2];
        const long long ts = std::stoll(parts[3]);
        HandleScore(player_id, score, ts);
    } else if (parts[0] == "STATE" && parts.size() >= 2) {
        ui_.SetScene(parts[1]);
    } else if (parts[0] == "UI" && parts.size() >= 3) {
        if (parts[1] == "SCENE" && parts.size() >= 3) {
            ui_.SetScene(parts[2]);
        } else if (parts[1] == "TEXT" && parts.size() >= 4) {
            ui_.SetText(parts[2], parts[3]);
        } else if (parts[1] == "IMAGE" && parts.size() >= 4) {
            const std::string path = cfg_.assets_dir + "/" + parts[3];
            ui_.SetImage(parts[2], path);
        }
    }
}

void App::HandleScore(const std::string& player_id, int score, long long timestamp) {
    ui_.SetText("score", std::to_string(score));
    ui_.SetScene("RESULTS");

    std::string video_path;
    if (!cfg_.camera_command.empty()) {
        const std::string filename = "hit_" + std::to_string(timestamp) + ".mp4";
        video_path = cfg_.video_dir + "/" + filename;
        video_.CaptureClip(video_path, cfg_.camera_duration_ms);
    }

    store::ScoreEntry entry{};
    entry.player_id = player_id;
    entry.score = score;
    entry.timestamp = timestamp;
    entry.video_path = video_path;
    entry.synced = 0;
    store_.AddScore(entry);

    RefreshLeaderboard();
}

void App::RefreshLeaderboard() {
    auto top = store_.GetTopScores(cfg_.leaderboard_size);
    std::vector<ui::ScoreEntry> ui_entries;
    for (const auto& e : top) {
        ui::ScoreEntry ue{};
        ue.player_id = e.player_id;
        ue.score = e.score;
        ue.timestamp = e.timestamp;
        ue.video_path = e.video_path;
        ui_entries.push_back(ue);
    }
    ui_.SetLeaderboard(ui_entries);
}

}  // namespace app
