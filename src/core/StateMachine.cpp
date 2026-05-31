#include "core/StateMachine.h"

#include "util/Logger.h"

namespace core {

const char* ToString(GameState state) {
    switch (state) {
        case GameState::Attract:
            return "ATTRACT";
        case GameState::ModeSelect:
            return "MODE_SELECT";
        case GameState::Measure:
            return "MEASURE";
        case GameState::EndGame:
            return "END_GAME";
        default:
            return "UNKNOWN";
    }
}

void StateMachine::Register(GameState state, std::unique_ptr<Screen> screen) {
    screens_[state] = std::move(screen);
}

void StateMachine::Start(GameState state, AppContext& ctx) {
    current_ = state;
    active_ = screens_.count(state) ? screens_[state].get() : nullptr;
    if (active_) {
        util::Log(util::LogLevel::Info, std::string("State -> ") + ToString(current_));
        active_->OnEnter(ctx);
    }
}

void StateMachine::Change(GameState next, AppContext& ctx) {
    if (next == current_ && active_) {
        return;
    }
    if (active_) {
        active_->OnExit(ctx);
    }
    Start(next, ctx);
}

void StateMachine::HandleEvent(const InputEvent& event, AppContext& ctx) {
    if (!active_) {
        return;
    }
    if (auto next = active_->HandleEvent(event, ctx)) {
        Change(*next, ctx);
    }
}

void StateMachine::Update(AppContext& ctx, double dt_ms) {
    if (!active_) {
        return;
    }
    if (auto next = active_->Update(ctx, dt_ms)) {
        Change(*next, ctx);
    }
}

void StateMachine::Render(AppContext& ctx) {
    if (active_) {
        active_->Render(ctx);
    }
}

}  // namespace core
