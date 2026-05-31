#pragma once

#include "ui/ScoreEntry.h"

#include <vector>

namespace ui {
class Renderer;
}

namespace ui::widgets {

// Stale elementy HUD: rekord automatu i liczba kredytow (lewy dolny rog).
void RenderHud(ui::Renderer& renderer, int credits, const std::vector<ui::ScoreEntry>& leaderboard);

}  // namespace ui::widgets
