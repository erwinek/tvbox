#pragma once

#include "core/Events.h"

#include <optional>
#include <string>

namespace io {

// Parsuje pojedyncza linie protokolu UART (ESP32 -> aplikacja) na zdarzenie wejscia.
// Obslugiwane:
//   COIN                           -> Coin (+1)
//   CREDIT,<n>                     -> Coin (text=abs, value=n) absolutny stan z PGM
//   START,<modeId>                 -> Start (text = boxer|kopacz|hammer)
//   HIT                            -> Hit (text=impact) moment uderzenia / stop kamery
//   SCORE,<value>,<player>,<ts>    -> Hit (value, player, ts)
//   STATE,<phase>,<mode>,<credit>  -> SyncState (text=phase:mode, value=credit)
//   STATE,HIT                      -> Hit (symulacja, value=0)
// Pozostale linie sa ignorowane (zwraca std::nullopt).
std::optional<core::InputEvent> ParseLine(const std::string& line);

}  // namespace io
