#pragma once

#include "ui/ScoreEntry.h"

#include <SDL.h>

#include <vector>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// HUD: RECORD / CREDIT — cyfra Huge nad etykieta Normal (wysrodkowane).
// Gdy podano `align`, bloki sa wyrownane do lewej/prawej krawedzi (jak TOP SCORES).
void RenderHud(ui::Renderer& renderer, int credits, const std::vector<ui::ScoreEntry>& leaderboard,
               const SDL_Rect* align = nullptr);

}  // namespace ui::widgets
