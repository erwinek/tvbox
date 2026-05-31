#pragma once

#include <SDL.h>

#include <map>
#include <string>

namespace ui {

// Cache tekstur obrazow ladowanych z plikow (po sciezce).
class TextureCache {
public:
    void SetRenderer(SDL_Renderer* renderer);
    SDL_Texture* Get(const std::string& path, int* out_w = nullptr, int* out_h = nullptr);
    void Clear();
    ~TextureCache();

private:
    struct Entry {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    SDL_Renderer* renderer_ = nullptr;
    std::map<std::string, Entry> cache_;
};

}  // namespace ui
