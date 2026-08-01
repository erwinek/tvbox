#pragma once

#include "core/Events.h"

#include <optional>
#include <string>

namespace io {

// Parsuje pojedyncza linie protokolu UART (ESP32 -> aplikacja) na zdarzenie wejscia.
// Obslugiwane:
//   COIN / CREDIT                  -> Coin
//   START,<modeId>                 -> Start (text = boxer|kopacz|hammer)
//   SCORE,<value>,<player>,<ts>    -> Hit (value, player, ts)
//   STATE,HIT                      -> Hit (symulacja, value=0)
// Pozostale linie sa ignorowane (zwraca std::nullopt).
std::optional<core::InputEvent> ParseLine(const std::string& line);

}  // namespace io
