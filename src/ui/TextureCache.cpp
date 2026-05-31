#include "ui/TextureCache.h"

#include <SDL_image.h>

namespace ui {

TextureCache::~TextureCache() {
    Clear();
}

void TextureCache::SetRenderer(SDL_Renderer* renderer) {
    renderer_ = renderer;
}

SDL_Texture* TextureCache::Get(const std::string& path, int* out_w, int* out_h) {
    auto it = cache_.find(path);
    if (it != cache_.end()) {
        if (out_w) *out_w = it->second.width;
        if (out_h) *out_h = it->second.height;
        return it->second.texture;
    }

    Entry entry{};
    if (renderer_) {
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (surface) {
            entry.texture = SDL_CreateTextureFromSurface(renderer_, surface);
            entry.width = surface->w;
            entry.height = surface->h;
            SDL_FreeSurface(surface);
        }
    }
    cache_[path] = entry;
    if (out_w) *out_w = entry.width;
    if (out_h) *out_h = entry.height;
    return entry.texture;
}

void TextureCache::Clear() {
    for (auto& kv : cache_) {
        if (kv.second.texture) {
            SDL_DestroyTexture(kv.second.texture);
        }
    }
    cache_.clear();
}

}  // namespace ui
