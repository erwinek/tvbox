#pragma once

#include <SDL.h>

#include <string>
#include <vector>

namespace ui {

// Laduje i odtwarza sekwencje klatek (frame_%04d.jpg) jako zapetlony klip.
class FrameSequencePlayer {
public:
    ~FrameSequencePlayer();

    bool Load(SDL_Renderer* renderer, const std::string& frames_dir, int max_frames = 300);
    void Clear();

    bool valid() const { return frame_count_ > 0; }
    int frame_count() const { return frame_count_; }
    int width() const { return width_; }
    int height() const { return height_; }
    const std::string& dir() const { return dir_; }

    SDL_Texture* FrameAt(Uint32 elapsed_ms, int fps) const;

private:
    std::string dir_;
    std::vector<SDL_Texture*> frames_;
    int width_ = 0;
    int height_ = 0;
    int frame_count_ = 0;
};

}  // namespace ui
