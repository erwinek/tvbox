#pragma once

#include <SDL_mixer.h>

#include <map>
#include <string>

namespace media {

// Prosty odtwarzacz dzwiekow i muzyki oparty o SDL2_mixer.
// Wszystkie metody sa bezpieczne gdy audio nie zostalo zainicjalizowane
// lub gdy dany dzwiek nie zostal zaladowany (no-op).
class AudioPlayer {
public:
    ~AudioPlayer();

    bool Init();
    void Shutdown();

    bool LoadSound(const std::string& key, const std::string& path);
    bool LoadMusic(const std::string& path);

    void PlaySound(const std::string& key);
    void PlayMusic(int loops = -1);
    void StopMusic();

    bool ready() const { return ready_; }

private:
    bool ready_ = false;
    Mix_Music* music_ = nullptr;
    std::map<std::string, Mix_Chunk*> sounds_;
};

}  // namespace media
