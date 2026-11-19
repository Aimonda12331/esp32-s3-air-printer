#include "file_manager.hpp"

#include <cstdio>
#include <cstring>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"

namespace printer {

static const char* TAG = "FileManager";
static const char* NVS_NAMESPACE = "app_cfg";
static const char* NVS_KEY_DOCS  = "docs_queue";

FileManager::FileManager() {
    loadFromNvs();
}

FileManager& FileManager::getInstance() {
    static FileManager instance;
    return instance;
}

esp_err_t FileManager::init() {
    std::lock_guard<std::mutex> lock(mutex_);
    loadFromNvs();
    ESP_LOGI(TAG, "FileManager đã khởi tạo, hiện có %zu tài liệu trong hàng đợi.", files_.size());
    return ESP_OK;
}

std::string FileManager::formatBytes(size_t bytes) {
    char buf[32];
    if (bytes >= 1024 * 1024) {
        std::snprintf(buf, sizeof(buf), "%.1f MB", (float)bytes / (1024.0f * 1024.0f));
    } else if (bytes >= 1024) {
        std::snprintf(buf, sizeof(buf), "%.1f KB", (float)bytes / 1024.0f);
    } else {
        std::snprintf(buf, sizeof(buf), "%u B", (unsigned int)bytes);
    }
    return std::string(buf);
}

bool FileManager::addFile(const std::string& name, const std::string& sizeStr) {
    if (name.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    bool exists = false;
    for (auto& doc : files_) {
        if (doc.name == name) {
            doc.sizeStr = sizeStr;
            exists = true;
            break;
        }
    }

    if (!exists) {
        files_.push_back({name, sizeStr});
    }

    saveToNvs();
    ESP_LOGI(TAG, "Đã thêm/cập nhật tài liệu: [%s] (%s)", name.c_str(), sizeStr.c_str());
    return true;
}

bool FileManager::addFile(const std::string& name, size_t bytes) {
    return addFile(name, formatBytes(bytes));
}

bool FileManager::removeFile(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = files_.begin(); it != files_.end(); ++it) {
        if (it->name == name) {
            files_.erase(it);
            saveToNvs();
            ESP_LOGI(TAG, "Đã xóa tài liệu khỏi hàng đợi & Flash NVS: %s", name.c_str());
            return true;
        }
    }
    return false;
}

void FileManager::clearAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    files_.clear();
    saveToNvs();
    ESP_LOGI(TAG, "Đã xóa sạch toàn bộ hàng đợi tài liệu.");
}

std::vector<PrintDocument> FileManager::getFiles() {
    std::lock_guard<std::mutex> lock(mutex_);
    return files_;
}

bool FileManager::hasFile(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& doc : files_) {
        if (doc.name == name) return true;
    }
    return false;
}

size_t FileManager::getFileCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    return files_.size();
}

void FileManager::saveToNvs() {
    cJSON* root = cJSON_CreateArray();
    for (const auto& doc : files_) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "n", doc.name.c_str());
        cJSON_AddStringToObject(item, "s", doc.sizeStr.c_str());
        cJSON_AddItemToArray(root, item);
    }

    char* jsonStr = cJSON_PrintUnformatted(root);
    if (jsonStr) {
        nvs_handle_t handle;
        if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
            nvs_set_str(handle, NVS_KEY_DOCS, jsonStr);
            nvs_commit(handle);
            nvs_close(handle);
        }
        cJSON_free(jsonStr);
    }
    cJSON_Delete(root);
}

void FileManager::loadFromNvs() {
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return;
    }

    size_t reqSize = 0;
    if (nvs_get_str(handle, NVS_KEY_DOCS, nullptr, &reqSize) != ESP_OK || reqSize <= 1) {
        nvs_close(handle);
        return;
    }

    std::vector<char> buf(reqSize);
    if (nvs_get_str(handle, NVS_KEY_DOCS, buf.data(), &reqSize) == ESP_OK) {
        cJSON* root = cJSON_Parse(buf.data());
        if (root && cJSON_IsArray(root)) {
            files_.clear();
            cJSON* item = nullptr;
            cJSON_ArrayForEach(item, root) {
                cJSON* jName = cJSON_GetObjectItem(item, "n");
                cJSON* jSize = cJSON_GetObjectItem(item, "s");
                if (jName && jName->valuestring && jSize && jSize->valuestring) {
                    files_.push_back({jName->valuestring, jSize->valuestring});
                }
            }
            ESP_LOGI(TAG, "Đã khôi phục %zu tài liệu từ Flash NVS.", files_.size());
        }
        if (root) cJSON_Delete(root);
    }
    nvs_close(handle);
}

} // namespace printer
