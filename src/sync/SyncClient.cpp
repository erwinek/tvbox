#include "sync/SyncClient.h"

#include "util/Logger.h"

#include <curl/curl.h>

namespace sync {

void SyncClient::Configure(const std::string& server_url, const std::string& auth_token) {
    server_url_ = server_url;
    auth_token_ = auth_token;
}

bool SyncClient::SyncOnce(store::LeaderboardStore& store, int batch_size) {
    if (server_url_.empty()) {
        util::Log(util::LogLevel::Warn, "Sync disabled: server_url is empty");
        return false;
    }
    auto entries = store.GetUnsynced(batch_size);
    bool any = false;
    for (const auto& entry : entries) {
        if (UploadEntry(entry)) {
            store.MarkSynced(entry.id);
            any = true;
        }
    }
    return any;
}

bool SyncClient::UploadEntry(const store::ScoreEntry& entry) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string url = server_url_ + "/scores";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "tvbox-gui/1.0");

    struct curl_slist* headers = nullptr;
    if (!auth_token_.empty()) {
        std::string auth = "Authorization: Bearer " + auth_token_;
        headers = curl_slist_append(headers, auth.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_mime* form = curl_mime_init(curl);
    curl_mimepart* field = curl_mime_addpart(form);
    curl_mime_name(field, "player_id");
    curl_mime_data(field, entry.player_id.c_str(), CURL_ZERO_TERMINATED);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "score");
    curl_mime_data(field, std::to_string(entry.score).c_str(), CURL_ZERO_TERMINATED);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "timestamp");
    curl_mime_data(field, std::to_string(entry.timestamp).c_str(), CURL_ZERO_TERMINATED);

    if (!entry.video_path.empty()) {
        field = curl_mime_addpart(form);
        curl_mime_name(field, "video");
        curl_mime_filedata(field, entry.video_path.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    const CURLcode res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    curl_mime_free(form);
    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || status < 200 || status >= 300) {
        util::Log(util::LogLevel::Warn, "Sync upload failed");
        return false;
    }
    return true;
}

}  // namespace sync
