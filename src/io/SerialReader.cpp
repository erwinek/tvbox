#include "io/SerialReader.h"

#include "util/Logger.h"

#include <chrono>
#include <thread>

#ifdef __linux__
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
#ifdef __linux__
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

bool SerialReader::OpenPort(const std::string& port, int baud_rate) {
#ifdef __linux__
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
#ifdef __linux__
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
#endif
}

}  // namespace io
