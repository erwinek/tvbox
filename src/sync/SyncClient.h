#pragma once

#include "store/LeaderboardStore.h"

#include <string>

namespace tvsync {

class SyncClient {
public:
    void Configure(const std::string& server_url, const std::string& auth_token);
    bool SyncOnce(store::LeaderboardStore& store, int batch_size);

private:
    bool UploadEntry(const store::ScoreEntry& entry);

    std::string server_url_;
    std::string auth_token_;
};

}  // namespace tvsync
