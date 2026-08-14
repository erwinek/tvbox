#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct SDL_Texture;
struct SDL_Rect;

namespace ui {

class Renderer;

// Petla tła bez live ffmpeg: raz dekoduje klip do RAM, potem tylko blit SDL.
// Live ffmpeg na Wyse zjada CPU razem z kamera.
class BackgroundPlayer {
public:
    BackgroundPlayer() = default;
    ~BackgroundPlayer();

    BackgroundPlayer(const BackgroundPlayer&) = delete;
    BackgroundPlayer& operator=(const BackgroundPlayer&) = delete;

    void Init(const std::string& background_dir);
    void Shutdown();

    void SetPlaying(bool playing);
    bool IsPlaying() const { return playing_; }

    // Rysuje aktualna klatke w dst. false = brak klatek.
    bool Render(Renderer& renderer, const SDL_Rect& dst);

private:
    void Preload(const std::string& path);
    static bool IsVideoExt(const std::string& ext);

    std::vector<std::string> clips_;
    std::vector<std::vector<std::uint8_t>> frames_;
    int frame_w_ = 0;
    int frame_h_ = 0;
    int play_fps_ = 8;
    bool playing_ = false;

    SDL_Texture* texture_ = nullptr;
    int tex_w_ = 0;
    int tex_h_ = 0;
    int tex_index_ = -1;
};

}  // namespace ui
