#include "io/SerialReader.h"

#include "util/Logger.h"

#include <chrono>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace io {

SerialReader::SerialReader() = default;

SerialReader::~SerialReader() {
    Stop();
}

bool SerialReader::Start(const std::string& port, int baud_rate, LineCallback callback) {
    if (running_) {
        return false;
    }
    callback_ = std::move(callback);
    if (!OpenPort(port, baud_rate)) {
        return false;
    }
    running_ = true;
    thread_ = std::thread(&SerialReader::ReadLoop, this);
    return true;
}

void SerialReader::Stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
    ClosePort();
}

void SerialReader::ReadLoop() {
    while (running_) {
#ifdef _WIN32
        if (port_handle_ == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        char buf[256];
        DWORD n = 0;
        if (ReadFile(port_handle_, buf, sizeof(buf), &n, nullptr) && n > 0) {
            buffer_.append(buf, buf + n);
            std::size_t pos = 0;
            while ((pos = buffer_.find('\n')) != std::string::npos) {
                std::string line = buffer_.substr(0, pos);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                buffer_.erase(0, pos + 1);
                if (callback_) {
                    callback_(line);
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
#elif defined(__linux__)
        char buf[256];
        const int n = static_cast<int>(read(fd_, buf, sizeof(buf)));
        if (n > 0) {
            buffer_.append(buf, buf + n);
            std::size_t pos = 0;
            while ((pos = buffer_.find('\n')) != std::string::npos) {
                std::string line = buffer_.substr(0, pos);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                buffer_.erase(0, pos + 1);
                if (callback_) {
                    callback_(line);
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
    }
}

#ifdef _WIN32
static DWORD BaudToWinRate(int baud_rate) {
    switch (baud_rate) {
        case 9600: return CBR_9600;
        case 57600: return CBR_57600;
        case 230400: return 230400;
        default: return CBR_115200;
    }
}
#endif

bool SerialReader::OpenPort(const std::string& port, int baud_rate) {
#ifdef _WIN32
    std::string device = port;
    if (device.rfind("\\\\.\\", 0) != 0) {
        device = "\\\\.\\" + port;
    }

    port_handle_ = CreateFileA(device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                               OPEN_EXISTING, 0, nullptr);
    if (port_handle_ == INVALID_HANDLE_VALUE) {
        util::Log(util::LogLevel::Error, "Serial: cannot open port " + port);
        return false;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(port_handle_, &dcb)) {
        util::Log(util::LogLevel::Error, "Serial: GetCommState failed");
        ClosePort();
        return false;
    }

    dcb.BaudRate = BaudToWinRate(baud_rate);
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(port_handle_, &dcb)) {
        util::Log(util::LogLevel::Error, "Serial: SetCommState failed");
        ClosePort();
        return false;
    }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    SetCommTimeouts(port_handle_, &timeouts);
    return true;
#elif defined(__linux__)
    fd_ = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        util::Log(util::LogLevel::Error, "Serial: cannot open port " + port);
        return false;
    }

    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) {
        util::Log(util::LogLevel::Error, "Serial: tcgetattr failed");
        ClosePort();
        return false;
    }

    cfmakeraw(&tty);
    speed_t speed = B115200;
    if (baud_rate == 9600) speed = B9600;
    if (baud_rate == 57600) speed = B57600;
    if (baud_rate == 230400) speed = B230400;
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        util::Log(util::LogLevel::Error, "Serial: tcsetattr failed");
        ClosePort();
        return false;
    }
    return true;
#else
    (void)port;
    (void)baud_rate;
    util::Log(util::LogLevel::Warn, "Serial: not supported on this platform");
    return false;
#endif
}

void SerialReader::ClosePort() {
#ifdef _WIN32
    if (port_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(port_handle_);
        port_handle_ = INVALID_HANDLE_VALUE;
    }
#elif defined(__linux__)
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
#endif
}

}  // namespace io
