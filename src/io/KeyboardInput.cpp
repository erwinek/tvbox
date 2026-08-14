#include "io/KeyboardInput.h"

#include "core/Clock.h"

namespace io {

std::optional<core::InputEvent> TranslateKey(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        core::InputEvent e{};
        e.type = core::InputType::Quit;
        return e;
    }

    if (event.type != SDL_KEYDOWN || event.key.repeat) {
        return std::nullopt;
    }

    core::InputEvent e{};
    e.ts = core::NowMs();

    switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
            e.type = core::InputType::Quit;
            return e;
        case SDLK_c:
            e.type = core::InputType::Coin;
            return e;
        case SDLK_SPACE:
            e.type = core::InputType::Hit;
            e.value = 0;  // symulacja
            return e;
        case SDLK_LEFT:
            e.type = core::InputType::SelectMode;
            e.value = -1;
            return e;
        case SDLK_RIGHT:
            e.type = core::InputType::SelectMode;
            e.value = 1;
            return e;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            e.type = core::InputType::Confirm;
            return e;
        case SDLK_BACKSPACE:
            e.type = core::InputType::Back;
            return e;
        case SDLK_v:
            e.type = core::InputType::PurgeRequest;
            return e;
        case SDLK_1:
        case SDLK_KP_1:
            e.type = core::InputType::DebugGoto;
            e.value = 1;
            return e;
        case SDLK_2:
        case SDLK_KP_2:
            e.type = core::InputType::DebugGoto;
            e.value = 2;
            return e;
        case SDLK_3:
        case SDLK_KP_3:
            e.type = core::InputType::DebugGoto;
            e.value = 3;
            return e;
        case SDLK_4:
        case SDLK_KP_4:
            e.type = core::InputType::DebugGoto;
            e.value = 4;
            return e;
        default:
            return std::nullopt;
    }
}

}  // namespace io
