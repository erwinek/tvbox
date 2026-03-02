#pragma once

#include <string>
#include <vector>

namespace store {

struct ScoreEntry {
    int id = 0;
    std::string player_id;
    int score = 0;
    long long timestamp = 0;
    std::string video_path;
    int synced = 0;
};

class LeaderboardStore {
public:
    bool Open(const std::string& db_path);
    void Close();

    bool AddScore(const ScoreEntry& entry);
    std::vector<ScoreEntry> GetTopScores(int limit);
    std::vector<ScoreEntry> GetUnsynced(int limit);
    bool MarkSynced(int id);

private:
    void EnsureSchema();
    void* db_ = nullptr;
};

}  // namespace store
