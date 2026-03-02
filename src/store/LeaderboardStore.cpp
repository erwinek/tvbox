#include "store/LeaderboardStore.h"

#include "util/Logger.h"

#include <sqlite3.h>

namespace store {

bool LeaderboardStore::Open(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), reinterpret_cast<sqlite3**>(&db_)) != SQLITE_OK) {
        util::Log(util::LogLevel::Error, "SQLite open failed");
        db_ = nullptr;
        return false;
    }
    EnsureSchema();
    return true;
}

void LeaderboardStore::Close() {
    if (db_) {
        sqlite3_close(reinterpret_cast<sqlite3*>(db_));
        db_ = nullptr;
    }
}

void LeaderboardStore::EnsureSchema() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS scores ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "player_id TEXT,"
        "score INTEGER,"
        "timestamp INTEGER,"
        "video_path TEXT,"
        "synced INTEGER DEFAULT 0"
        ");";
    char* err = nullptr;
    sqlite3_exec(reinterpret_cast<sqlite3*>(db_), sql, nullptr, nullptr, &err);
    if (err) {
        util::Log(util::LogLevel::Warn, std::string("SQLite schema: ") + err);
        sqlite3_free(err);
    }
}

bool LeaderboardStore::AddScore(const ScoreEntry& entry) {
    const char* sql =
        "INSERT INTO scores (player_id, score, timestamp, video_path, synced) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, entry.player_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, entry.score);
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(entry.timestamp));
    sqlite3_bind_text(stmt, 4, entry.video_path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, entry.synced);
    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<ScoreEntry> LeaderboardStore::GetTopScores(int limit) {
    std::vector<ScoreEntry> out;
    const char* sql =
        "SELECT id, player_id, score, timestamp, video_path, synced "
        "FROM scores ORDER BY score DESC, timestamp DESC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return out;
    }
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ScoreEntry e{};
        e.id = sqlite3_column_int(stmt, 0);
        e.player_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        e.score = sqlite3_column_int(stmt, 2);
        e.timestamp = sqlite3_column_int64(stmt, 3);
        const unsigned char* vid = sqlite3_column_text(stmt, 4);
        e.video_path = vid ? reinterpret_cast<const char*>(vid) : "";
        e.synced = sqlite3_column_int(stmt, 5);
        out.push_back(e);
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<ScoreEntry> LeaderboardStore::GetUnsynced(int limit) {
    std::vector<ScoreEntry> out;
    const char* sql =
        "SELECT id, player_id, score, timestamp, video_path, synced "
        "FROM scores WHERE synced = 0 ORDER BY timestamp ASC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return out;
    }
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ScoreEntry e{};
        e.id = sqlite3_column_int(stmt, 0);
        e.player_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        e.score = sqlite3_column_int(stmt, 2);
        e.timestamp = sqlite3_column_int64(stmt, 3);
        const unsigned char* vid = sqlite3_column_text(stmt, 4);
        e.video_path = vid ? reinterpret_cast<const char*>(vid) : "";
        e.synced = sqlite3_column_int(stmt, 5);
        out.push_back(e);
    }
    sqlite3_finalize(stmt);
    return out;
}

bool LeaderboardStore::MarkSynced(int id) {
    const char* sql = "UPDATE scores SET synced = 1 WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(reinterpret_cast<sqlite3*>(db_), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, id);
    const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

}  // namespace store
