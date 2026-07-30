#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct SDL_Texture;

namespace ui {

class Renderer;

// Odtwarza filmy z katalogu (mp4/mkv/webm/avi) w petli jako tlo UI.
// Dekodowanie przez ffmpeg CLI -> surowe RGB24 do tekstury SDL.
class BackgroundPlayer {
public:
    BackgroundPlayer() = default;
    ~BackgroundPlayer();

    BackgroundPlayer(const BackgroundPlayer&) = delete;
    BackgroundPlayer& operator=(const BackgroundPlayer&) = delete;

    void Init(const std::string& background_dir);
    void Shutdown();

    // Rysuje aktualna klatke na caly design-space. false = brak tla.
    bool Render(Renderer& renderer);

private:
    void ReaderLoop();
    bool StartClip(const std::string& path);
    void StopClip();
    void AdvanceClip();
    static bool IsVideoExt(const std::string& ext);

    std::vector<std::string> clips_;
    std::size_t clip_index_ = 0;

    std::thread reader_thread_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> clip_ended_{false};
    FILE* pipe_ = nullptr;

    int frame_w_ = 0;
    int frame_h_ = 0;
    std::vector<std::uint8_t> latest_frame_;
    bool frame_ready_ = false;
    mutable std::mutex frame_mutex_;

    SDL_Texture* texture_ = nullptr;
    int tex_w_ = 0;
    int tex_h_ = 0;
};

}  // namespace ui
