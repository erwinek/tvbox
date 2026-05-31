#include "ui/FrameSequencePlayer.h"

#include "util/Logger.h"

#include <SDL_image.h>

#include <cstdio>

namespace ui {

FrameSequencePlayer::~FrameSequencePlayer() {
    Clear();
}

bool FrameSequencePlayer::Load(SDL_Renderer* renderer, const std::string& frames_dir, int max_frames) {
    Clear();
    dir_ = frames_dir;
    if (!renderer) {
        return false;
    }

    for (int i = 1; i <= max_frames; ++i) {
        char filename[64];
        std::snprintf(filename, sizeof(filename), "/frame_%04d.jpg", i);
        const std::string path = frames_dir + filename;
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            break;
        }
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        if (i == 1) {
            width_ = surface->w;
            height_ = surface->h;
        }
        SDL_FreeSurface(surface);
        if (!tex) {
            break;
        }
        frames_.push_back(tex);
    }

    frame_count_ = static_cast<int>(frames_.size());
    if (frame_count_ > 0) {
        util::Log(util::LogLevel::Info,
                  "Loaded " + std::to_string(frame_count_) + " frames from " + frames_dir);
    }
    return frame_count_ > 0;
}

void FrameSequencePlayer::Clear() {
    for (auto* tex : frames_) {
        if (tex) {
            SDL_DestroyTexture(tex);
        }
    }
    frames_.clear();
    frame_count_ = 0;
    width_ = 0;
    height_ = 0;
    dir_.clear();
}

SDL_Texture* FrameSequencePlayer::FrameAt(Uint32 elapsed_ms, int fps) const {
    if (frame_count_ <= 0 || fps <= 0) {
        return nullptr;
    }
    const int idx = static_cast<int>((elapsed_ms / (1000 / fps))) % frame_count_;
    return frames_[idx];
}

}  // namespace ui
