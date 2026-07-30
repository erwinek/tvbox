#pragma once

#include "ui/ScoreEntry.h"

#include <SDL.h>

#include <vector>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// HUD: RECORD (lewa) i CREDIT (prawa).
// Gdy podano `align`, pillki sa wyrownane do lewej/prawej krawedzi (jak TOP SCORES).
void RenderHud(ui::Renderer& renderer, int credits, const std::vector<ui::ScoreEntry>& leaderboard,
               const SDL_Rect* align = nullptr);

}  // namespace ui::widgets
