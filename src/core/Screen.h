#pragma once

#include "core/Events.h"
#include "core/GameState.h"

#include <optional>

namespace core {

struct AppContext;

// Bazowy interfejs ekranu (jednego stanu maszyny stanow).
// Metody zwracajace std::optional<GameState> moga zazadac przejscia do innego stanu.
class Screen {
public:
    virtual ~Screen() = default;

    virtual void OnEnter(AppContext& ctx) { (void)ctx; }
    virtual void OnExit(AppContext& ctx) { (void)ctx; }

    virtual std::optional<GameState> HandleEvent(const InputEvent& event, AppContext& ctx) = 0;
    virtual std::optional<GameState> Update(AppContext& ctx, double dt_ms) = 0;
    virtual void Render(AppContext& ctx) = 0;
};

}  // namespace core
