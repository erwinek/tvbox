#include "media/AudioPlayer.h"

#include "util/Logger.h"

namespace media {

AudioPlayer::~AudioPlayer() {
    Shutdown();
}

bool AudioPlayer::Init() {
    if (ready_) {
        return true;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        util::Log(util::LogLevel::Warn, std::string("Audio init failed: ") + Mix_GetError());
        return false;
    }
    ready_ = true;
    return true;
}

void AudioPlayer::Shutdown() {
    for (auto& kv : sounds_) {
        if (kv.second) {
            Mix_FreeChunk(kv.second);
        }
    }
    sounds_.clear();
    if (music_) {
        Mix_FreeMusic(music_);
        music_ = nullptr;
    }
    if (ready_) {
        Mix_CloseAudio();
        ready_ = false;
    }
}

bool AudioPlayer::LoadSound(const std::string& key, const std::string& path) {
    if (!ready_ || path.empty()) {
        return false;
    }
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        util::Log(util::LogLevel::Warn, "Audio: cannot load sound " + path);
        return false;
    }
    sounds_[key] = chunk;
    return true;
}

bool AudioPlayer::LoadMusic(const std::string& path) {
    if (!ready_ || path.empty()) {
        return false;
    }
    if (music_) {
        Mix_FreeMusic(music_);
        music_ = nullptr;
    }
    music_ = Mix_LoadMUS(path.c_str());
    if (!music_) {
        util::Log(util::LogLevel::Warn, "Audio: cannot load music " + path);
        return false;
    }
    return true;
}

void AudioPlayer::PlaySound(const std::string& key) {
    if (!ready_) {
        return;
    }
    auto it = sounds_.find(key);
    if (it != sounds_.end() && it->second) {
        Mix_PlayChannel(-1, it->second, 0);
    }
}

void AudioPlayer::PlayMusic(int loops) {
    if (ready_ && music_) {
        Mix_PlayMusic(music_, loops);
    }
}

void AudioPlayer::StopMusic() {
    if (ready_) {
        Mix_HaltMusic();
    }
}

}  // namespace media
