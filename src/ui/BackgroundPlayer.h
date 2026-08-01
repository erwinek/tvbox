#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct SDL_Texture;
struct SDL_Rect;

namespace ui {

class Renderer;

// Odtwarza film z katalogu (mp4/...) — uzywany jako male okienko (np. ekran Press Start).
// Dekodowanie przez ffmpeg CLI -> RGB24. Start dopiero po SetPlaying(true).
class BackgroundPlayer {
public:
    BackgroundPlayer() = default;
    ~BackgroundPlayer();

    BackgroundPlayer(const BackgroundPlayer&) = delete;
    BackgroundPlayer& operator=(const BackgroundPlayer&) = delete;

    void Init(const std::string& background_dir);
    void Shutdown();

    // Wlacza/wylacza ffmpeg (oszczedza CPU gdy ekran nie potrzebuje wideo).
    void SetPlaying(bool playing);
    bool IsPlaying() const { return playing_; }

    // Rysuje aktualna klatke w dst (wymagane). false = brak klatki / nie gra.
    bool Render(Renderer& renderer, const SDL_Rect& dst);

private:
    void ReaderLoop();
    bool StartClip(const std::string& path);
    void StopClip();
    void AdvanceClip();
    static bool IsVideoExt(const std::string& ext);

    std::vector<std::string> clips_;
    std::size_t clip_index_ = 0;
    bool playing_ = false;

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
