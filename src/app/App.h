#pragma once

#include "config/Config.h"
#include "io/SerialReader.h"
#include "media/VideoCapture.h"
#include "store/LeaderboardStore.h"
#include "sync/SyncClient.h"
#include "ui/UiRenderer.h"

#include <atomic>
#include <string>

namespace app {

class App {
public:
    explicit App(const config::Config& cfg);
    ~App();

    bool Init();
    void Run();

private:
    void HandleLine(const std::string& line);
    void HandleScore(const std::string& player_id, int score, long long timestamp);
    void RefreshLeaderboard();

    config::Config cfg_;
    io::SerialReader serial_;
    ui::UiRenderer ui_;
    store::LeaderboardStore store_;
    media::VideoCapture video_;
    sync::SyncClient sync_;

    std::atomic<bool> running_{false};
    long long last_sync_ms_ = 0;
};

}  // namespace app
