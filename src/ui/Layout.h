#pragma once

#include <SDL.h>

namespace ui {

// Mapuje wspolrzedne design-space (np. 1920x1080) na piksele ekranu z uniform scale
// i opcjonalnymi pasami (letterbox) gdy proporcje sie roznia.
struct Layout {
    int design_w = 1920;
    int design_h = 1080;
    int actual_w = 1920;
    int actual_h = 1080;
    float scale = 1.f;
    int offset_x = 0;
    int offset_y = 0;

    // scale_override <= 0 => auto (min(actual/design)); > 0 => wymuszona skala.
    static Layout Create(int design_w, int design_h, int actual_w, int actual_h,
                         float scale_override = 0.f);

    int CenterX() const { return design_w / 2; }
    int CenterY() const { return design_h / 2; }
    float FontScale() const { return scale; }

    // Orientacja design-space (monitor docelowy jest pionowy).
    bool IsPortrait() const { return design_h > design_w; }
    int MinDim() const { return design_w < design_h ? design_w : design_h; }

    // Helpery procentowe (frakcje 0..1 w design-space).
    int PW(float frac) const;  // frac * design_w
    int PH(float frac) const;  // frac * design_h
    int PM(float frac) const;  // frac * min(design_w, design_h) - pady, promienie, ikony
    SDL_Rect PRect(float x, float y, float w, float h) const;  // wszystko we frakcjach

    int X(int design_px) const;
    int Y(int design_px) const;
    int S(int design_px) const;
    SDL_Rect Rect(int x, int y, int w, int h) const;
};

}  // namespace ui
