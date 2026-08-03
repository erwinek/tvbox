#pragma once

#include <SDL.h>

#include <string>
#include <vector>

namespace ui {

// Laduje i odtwarza sekwencje klatek (frame_%04d.jpg) jako zapetlony klip.
// Ladowanie jest przyrostowe (PumpLoad), zeby nie blokowac renderu / scrolla.
class FrameSequencePlayer {
public:
    ~FrameSequencePlayer();

    // Pelne synchroniczne ladowanie (np. EndGame replay).
    bool Load(SDL_Renderer* renderer, const std::string& frames_dir, int max_frames = 300);

    // Start przyrostowego ladowania (idempotentne dla tego samego dir).
    void BeginLoad(SDL_Renderer* renderer, const std::string& frames_dir, int max_frames = 300);

    // Doladuj do `budget` kolejnych klatek. Zwraca true gdy nadal trwa ladowanie.
    bool PumpLoad(int budget);

    void Clear();

    bool valid() const { return frame_count_ > 0; }
    bool ready() const { return frame_count_ > 0 && !loading_; }
    bool loading() const { return loading_; }
    int frame_count() const { return frame_count_; }
    int width() const { return width_; }
    int height() const { return height_; }
    const std::string& dir() const { return dir_; }

    SDL_Texture* FrameAt(Uint32 elapsed_ms, int fps) const;

private:
    SDL_Renderer* renderer_ = nullptr;
    std::string dir_;
    std::vector<SDL_Texture*> frames_;
    int width_ = 0;
    int height_ = 0;
    int frame_count_ = 0;
    int max_frames_ = 300;
    int next_index_ = 1;  // kolejny numer pliku frame_XXXX
    bool loading_ = false;
};

}  // namespace ui
