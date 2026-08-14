#pragma once

#include <string>

namespace core {

enum class InputType {
    Coin,        // wrzucony kredyt / CREDIT abs (text=abs)
    SelectMode,  // zmiana wyboru trybu (value = kierunek -1/+1)
    Confirm,     // zatwierdzenie / start (dev/klawiatura)
    Start,       // START z PGM: text = mode id (boxer|kopacz|hammer)
    Back,        // cofnij
    Hit,         // uderzenie (value = zmierzona sila, <=0 => symulacja; text = player id | impact)
    SyncState,   // STATE heartbeat: text = "phase:mode", value = credit
    Quit,        // wyjscie z aplikacji
    DebugGoto,   // dev: wymuszone przejscie do stanu (value = numer stanu 1..4)
    PurgeRequest  // serwis: zadanie skasowania wszystkich rekordow i nagran (klawisz V)
};

struct InputEvent {
    InputType type;
    int value = 0;
    std::string text;
    long long ts = 0;
};

}  // namespace core
