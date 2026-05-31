#pragma once

#include "core/Events.h"
#include "core/GameState.h"
#include "core/Screen.h"

#include <map>
#include <memory>

namespace core {

struct AppContext;

class StateMachine {
public:
    void Register(GameState state, std::unique_ptr<Screen> screen);
    void Start(GameState state, AppContext& ctx);

    void HandleEvent(const InputEvent& event, AppContext& ctx);
    void Update(AppContext& ctx, double dt_ms);
    void Render(AppContext& ctx);

    // Wymuszone przejscie do stanu (np. symulacja UART / debug).
    void GoTo(GameState next, AppContext& ctx) { Change(next, ctx); }

    GameState current() const { return current_; }

private:
    void Change(GameState next, AppContext& ctx);

    std::map<GameState, std::unique_ptr<Screen>> screens_;
    GameState current_ = GameState::Attract;
    Screen* active_ = nullptr;
};

}  // namespace core
