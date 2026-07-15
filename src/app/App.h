#pragma once

#include "config/Config.h"
#include "core/AppContext.h"
#include "core/InputQueue.h"
#include "core/StateMachine.h"
#include "game/GameSession.h"
#include "io/SerialInput.h"
#include "media/AudioPlayer.h"
#include "media/VideoCapture.h"
#include "store/LeaderboardStore.h"
#include "sync/SyncClient.h"
#include "ui/Renderer.h"

#include <atomic>
#include <string>
#include <thread>

namespace app {

class App {
public:
    explicit App(const config::Config& cfg);
    ~App();

    bool Init();
    void Run();

private:
    void RegisterScreens();
    void CommitScore(const std::string& player_id, int score);
    void StartMeasureRecording();
    void ScheduleHitRecordingFinalize(const std::string& video_path);
    void CancelMeasureRecording();
    void PollRecordingFinalize();
    void RefreshLeaderboard();

    config::Config cfg_;

    ui::Renderer renderer_;
    media::AudioPlayer audio_;
    media::VideoCapture video_;
    store::LeaderboardStore store_;
    tvsync::SyncClient sync_;
    game::GameSession session_;

    io::SerialInput serial_;
    core::InputQueue input_queue_;
    core::StateMachine fsm_;
    core::AppContext ctx_;

    std::atomic<bool> running_{false};
    std::atomic<bool> recording_{false};
    std::atomic<bool> leaderboard_dirty_{false};
    long long last_sync_ms_ = 0;
    long long finalize_at_ms_ = 0;
    std::string pending_video_path_;
    bool hit_finalize_scheduled_ = false;
    std::thread capture_thread_;
};

}  // namespace app
