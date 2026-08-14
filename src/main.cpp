#include "app/App.h"
#include "config/Config.h"
#include "util/Logger.h"
#include "version.h"

#include <cstring>
#include <iostream>

int main(int argc, char** argv) {
    if (argc > 1 && (std::strcmp(argv[1], "--version") == 0 || std::strcmp(argv[1], "-v") == 0)) {
        std::cout << tvbox::VersionString() << "  " << tvbox::VersionDetail() << std::endl;
        return 0;
    }

    std::string config_path = "config/app.yaml";
    if (argc > 1) {
        config_path = argv[1];
    }

    const config::Config cfg = config::LoadConfig(config_path);
    app::App app(cfg);
    if (!app.Init()) {
        util::Log(util::LogLevel::Error, "App init failed");
        return 1;
    }
    app.Run();
    return 0;
}
