#pragma once

#include "core/InputQueue.h"
#include "io/SerialReader.h"

#include <string>

namespace io {

// Zrodlo zdarzen z UART: czyta linie przez SerialReader, parsuje protokolem
// i wrzuca gotowe zdarzenia do wspolnej kolejki.
class SerialInput {
public:
    bool Start(const std::string& port, int baud_rate, core::InputQueue& queue);
    void Stop();

private:
    SerialReader reader_;
};

}  // namespace io
