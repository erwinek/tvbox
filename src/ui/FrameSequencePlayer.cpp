#include "ui/FrameSequencePlayer.h"

#include "util/Logger.h"

#include <SDL_image.h>

#include <cstdio>

namespace ui {

FrameSequencePlayer::~FrameSequencePlayer() {
    Clear();
}

bool FrameSequencePlayer::Load(SDL_Renderer* renderer, const std::string& frames_dir, int max_frames) {
    BeginLoad(renderer, frames_dir, max_frames);
    while (PumpLoad(64)) {
    }
    return frame_count_ > 0;
}

void FrameSequencePlayer::BeginLoad(SDL_Renderer* renderer, const std::string& frames_dir,
                                    int max_frames) {
    if (!renderer || frames_dir.empty()) {
        return;
    }
    // Juz ladujemy / zaladowane ten sam katalog — nic nie rob.
    if (dir_ == frames_dir && (loading_ || frame_count_ > 0)) {
        return;
    }

    Clear();
    renderer_ = renderer;
    dir_ = frames_dir;
    max_frames_ = max_frames > 0 ? max_frames : 300;
    next_index_ = 1;
    loading_ = true;
}

bool FrameSequencePlayer::PumpLoad(int budget) {
    if (!loading_ || !renderer_ || dir_.empty() || budget <= 0) {
        return false;
    }

    int loaded = 0;
    while (loaded < budget && next_index_ <= max_frames_) {
        char filename[64];
        std::snprintf(filename, sizeof(filename), "/frame_%04d.jpg", next_index_);
        const std::string path = dir_ + filename;
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            loading_ = false;
            break;
        }
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_, surface);
        if (next_index_ == 1) {
            width_ = surface->w;
            height_ = surface->h;
        }
        SDL_FreeSurface(surface);
        if (!tex) {
            loading_ = false;
            break;
        }
        frames_.push_back(tex);
        ++next_index_;
        ++loaded;
    }

    frame_count_ = static_cast<int>(frames_.size());
    if (next_index_ > max_frames_) {
        loading_ = false;
    }

    if (!loading_ && frame_count_ > 0) {
        util::Log(util::LogLevel::Info,
                  "Loaded " + std::to_string(frame_count_) + " frames from " + dir_);
    }
    return loading_;
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
    renderer_ = nullptr;
    max_frames_ = 300;
    next_index_ = 1;
    loading_ = false;
}

SDL_Texture* FrameSequencePlayer::FrameAt(Uint32 elapsed_ms, int fps) const {
    if (frame_count_ <= 0 || fps <= 0) {
        return nullptr;
    }
    const int idx = static_cast<int>((elapsed_ms / (1000 / fps))) % frame_count_;
    return frames_[idx];
}

}  // namespace ui
