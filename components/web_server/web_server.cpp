#include "web_server.hpp"
#include "web_page.h"
#include "wifi_manager.hpp"
#include "file_manager.hpp"

#include <cstring>
#include <cstdlib>
#include <memory>
#include "esp_log.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"

namespace printer {

static const char* TAG = "WebServer";

static std::string urlDecode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            char hex[3] = { in[i+1], in[i+2], '\0' };
            char* end = nullptr;
            long val = strtol(hex, &end, 16);
            if (end == hex + 2) {
                out += static_cast<char>(val);
                i += 2;
                continue;
            }
        } else if (in[i] == '+') {
            out += ' ';
        } else {
            out += in[i];
        }
    }
    return out;
}

WebServer::~WebServer() {
    stop();
}

WebServer& WebServer::getInstance() {
    static WebServer instance;
    return instance;
}

std::string WebServer::loadDeviceNameFromNvs() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("app_cfg", NVS_READONLY, &handle);
    if (err == ESP_OK) {
        char buf[64];
        size_t len = sizeof(buf);
        err = nvs_get_str(handle, "dev_name", buf, &len);
        nvs_close(handle);
        if (err == ESP_OK && len > 1) {
            return std::string(buf);
        }
    }
    // Mặc định: air-printer-xxxx (theo MAC)
    return "air-printer-" + WifiManager::getInstance().getMacSuffix();
}

esp_err_t WebServer::saveDeviceNameToNvs(const std::string& name) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("app_cfg", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_str(handle, "dev_name", name.c_str());
    if (err == ESP_OK) {
        nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

std::string WebServer::getDeviceName() {
    if (deviceName_.empty()) {
        deviceName_ = loadDeviceNameFromNvs();
    }
    return deviceName_;
}

esp_err_t WebServer::setDeviceName(const std::string& name) {
    if (name.empty()) return ESP_ERR_INVALID_ARG;
    deviceName_ = name;
    saveDeviceNameToNvs(name);
    updateMdns(name);
    return ESP_OK;
}

void WebServer::updateMdns(const std::string& hostname) {
    static bool mdnsInitialized = false;
    if (!mdnsInitialized) {
        esp_err_t err = mdns_init();
        if (err == ESP_OK) {
            mdnsInitialized = true;
            mdns_service_add(nullptr, "_http", "_tcp", 80, nullptr, 0);
        } else {
            ESP_LOGW(TAG, "mdns_init thất bại!");
            return;
        }
    }

    mdns_hostname_set(hostname.c_str());
    mdns_instance_name_set(hostname.c_str());
    ESP_LOGI(TAG, "mDNS đã đăng ký tên: http://%s.local", hostname.c_str());
}

esp_err_t WebServer::start(uint16_t port) {
    if (serverHandle_ != nullptr) {
        return ESP_OK;
    }

    // Khởi tạo tên máy in và mDNS
    std::string name = getDeviceName();
    updateMdns(name);

    // Khởi tạo FileManager quản lý hàng đợi tệp tin & Flash NVS
    FileManager::getInstance().init();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.stack_size = 10240; // Tăng stack lên 10KB (mặc định 4KB) để tránh tràn bộ đệm FreeRTOS khi tải file lớn
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;

    if (httpd_start(&serverHandle_, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Khởi chạy HTTP Server thất bại!");
        return ESP_FAIL;
    }

    // 1. URI Trang chủ giao diện HTML
    httpd_uri_t uriIndex = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = &WebServer::handleIndexHtml,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(serverHandle_, &uriIndex);

    // 2. URI API quét Wi-Fi
    httpd_uri_t uriScan = {
        .uri      = "/api/wifi/scan",
        .method   = HTTP_GET,
        .handler  = &WebServer::handleWifiScan,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(serverHandle_, &uriScan);

    // 3. URI API lưu cấu hình (Tên máy in + Wi-Fi)
    httpd_uri_t uriConfig = {
        .uri      = "/api/config",
        .method   = HTTP_POST,
        .handler  = &WebServer::handleConfig,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(serverHandle_, &uriConfig);

    // 4. URI API lấy trạng thái hiện tại
    httpd_uri_t uriStatus = {
        .uri      = "/api/status",
        .method   = HTTP_GET,
        .handler  = &WebServer::handleStatus,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(serverHandle_, &uriStatus);

    // 5. URI API tải file lên
    httpd_uri_t uriUpload = {
        .uri      = "/api/upload",
        .method   = HTTP_POST,
        .handler  = &WebServer::handleUpload,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(serverHandle_, &uriUpload);

    // 6. URI API danh sách file (GET)
    httpd_uri_t uriFilesGet = {
        .uri      = "/api/files",
        .method   = HTTP_GET,
        .handler  = &WebServer::handleFiles,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(serverHandle_, &uriFilesGet);

    // 7. URI API xóa file (DELETE)
    httpd_uri_t uriFilesDel = {
        .uri      = "/api/files",
        .method   = HTTP_DELETE,
        .handler  = &WebServer::handleFiles,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(serverHandle_, &uriFilesDel);

    // 8. URI API ra lệnh in (POST)
    httpd_uri_t uriPrint = {
        .uri      = "/api/print",
        .method   = HTTP_POST,
        .handler  = &WebServer::handlePrint,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(serverHandle_, &uriPrint);

    ESP_LOGI(TAG, "Web Server đang chạy trên cổng %d", port);
    return ESP_OK;
}

esp_err_t WebServer::stop() {
    if (serverHandle_) {
        httpd_stop(serverHandle_);
        serverHandle_ = nullptr;
    }
    return ESP_OK;
}

esp_err_t WebServer::handleIndexHtml(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleWifiScan(httpd_req_t* req) {
    auto& wifi = WifiManager::getInstance();
    auto list = wifi.scanNetworks();

    cJSON* root = cJSON_CreateArray();
    for (const auto& ap : list) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ssid", ap.ssid.c_str());
        cJSON_AddNumberToObject(item, "rssi", ap.rssi);
        cJSON_AddBoolToObject(item, "locked", ap.isLocked);
        cJSON_AddItemToArray(root, item);
    }

    char* jsonStr = cJSON_PrintUnformatted(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, jsonStr);

    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t WebServer::handleConfig(httpd_req_t* req) {
    char buf[512];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    cJSON* root = cJSON_Parse(buf);
    if (!root) return ESP_FAIL;

    cJSON* jDevName = cJSON_GetObjectItem(root, "device_name");
    cJSON* jSsid    = cJSON_GetObjectItem(root, "ssid");
    cJSON* jPass    = cJSON_GetObjectItem(root, "password");

    std::string devName = jDevName && jDevName->valuestring ? jDevName->valuestring : "";
    std::string ssid    = jSsid && jSsid->valuestring ? jSsid->valuestring : "";
    std::string pass    = jPass && jPass->valuestring ? jPass->valuestring : "";
    cJSON_Delete(root);

    // 1. Cập nhật tên thiết bị nếu có
    if (!devName.empty()) {
        WebServer::getInstance().setDeviceName(devName);
    }

    // 2. Kết nối tới Wi-Fi
    auto& wifi = WifiManager::getInstance();
    esp_err_t connRet = wifi.connect(ssid, pass, 15000);

    cJSON* resp = cJSON_CreateObject();
    if (connRet == ESP_OK) {
        // Lưu thông tin Wi-Fi vào Flash NVS để tự động kết nối ở các lần khởi động sau
        wifi.saveCredentials(ssid, pass);

        cJSON_AddStringToObject(resp, "status", "connected");
        cJSON_AddStringToObject(resp, "ip", wifi.getIpString().c_str());
        cJSON_AddStringToObject(resp, "hostname", WebServer::getInstance().getDeviceName().c_str());
    } else {
        cJSON_AddStringToObject(resp, "status", "failed");
    }

    char* respStr = cJSON_PrintUnformatted(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, respStr);

    cJSON_free(respStr);
    cJSON_Delete(resp);
    return ESP_OK;
}

esp_err_t WebServer::handleStatus(httpd_req_t* req) {
    auto& wifi = WifiManager::getInstance();
    cJSON* resp = cJSON_CreateObject();

    cJSON_AddStringToObject(resp, "device_name", WebServer::getInstance().getDeviceName().c_str());
    cJSON_AddStringToObject(resp, "ip", wifi.getIpString().c_str());
    cJSON_AddStringToObject(resp, "status", wifi.getState() == WifiState::CONNECTED ? "connected" : "disconnected");

    char* respStr = cJSON_PrintUnformatted(resp);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, respStr);

    cJSON_free(respStr);
    cJSON_Delete(resp);
    return ESP_OK;
}

esp_err_t WebServer::handleUpload(httpd_req_t* req) {
    std::string filename = "tai_lieu_chua_dat_ten";

    // 1. Kiểm tra tiêu đề HTTP header 'X-Filename'
    char hdrVal[512] = {0};
    if (httpd_req_get_hdr_value_str(req, "X-Filename", hdrVal, sizeof(hdrVal)) == ESP_OK && hdrVal[0] != '\0') {
        filename = urlDecode(hdrVal);
    } else {
        // 2. Tìm trực tiếp trong req->uri để không bị giới hạn bộ đệm
        const char* p = strstr(req->uri, "filename=");
        if (p) {
            p += 9; // Bỏ qua "filename="
            const char* end = strchr(p, '&');
            std::string rawName = end ? std::string(p, end - p) : std::string(p);
            if (!rawName.empty()) {
                filename = urlDecode(rawName);
            }
        }
    }

    if (filename.empty()) {
        filename = "tai_lieu_chua_dat_ten";
    }

    size_t totalBytes = req->content_len;
    ESP_LOGI(TAG, "Bắt đầu nhận tải file: [%s], dung lượng: %zu bytes", filename.c_str(), totalBytes);

    // Cấp phát bộ đệm nhận stream trên Heap để không chiếm dụng task stack
    constexpr size_t CHUNK_SIZE = 4096;
    auto chunk = std::make_unique<char[]>(CHUNK_SIZE);
    size_t received = 0;

    while (received < totalBytes) {
        int toRead = (totalBytes - received > CHUNK_SIZE) ? CHUNK_SIZE : (totalBytes - received);
        int ret = httpd_req_recv(req, chunk.get(), toRead);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "Lỗi đọc dữ liệu tải lên");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        received += ret;
        // Dữ liệu nhị phân đang được stream qua thành công.
        // Khi tích hợp thẻ nhớ SD hoặc bộ đệm in nhiệt, chunk sẽ được ghi tiếp vào đây.
    }

    // Lưu vào hàng đợi và Flash NVS thông qua FileManager
    FileManager::getInstance().addFile(filename, totalBytes);

    // Yêu cầu trình duyệt đóng kết nối TCP cũ để reset socket sạch sẽ cho tệp tiếp theo
    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
}

esp_err_t WebServer::handleFiles(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        auto files = FileManager::getInstance().getFiles();
        cJSON* root = cJSON_CreateArray();
        for (const auto& doc : files) {
            cJSON* item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "name", doc.name.c_str());
            cJSON_AddStringToObject(item, "size", doc.sizeStr.c_str());
            cJSON_AddItemToArray(root, item);
        }

        char* jsonStr = cJSON_PrintUnformatted(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, jsonStr ? jsonStr : "[]");

        if (jsonStr) cJSON_free(jsonStr);
        cJSON_Delete(root);
        return ESP_OK;
    } else if (req->method == HTTP_DELETE) {
        std::string filename;

        // Tìm tên file trực tiếp từ URL không bị giới hạn bộ đệm
        const char* p = strstr(req->uri, "filename=");
        if (p) {
            p += 9;
            const char* end = strchr(p, '&');
            std::string rawName = end ? std::string(p, end - p) : std::string(p);
            if (!rawName.empty()) {
                filename = urlDecode(rawName);
            }
        }

        if (!filename.empty()) {
            FileManager::getInstance().removeFile(filename);
        }

        httpd_resp_set_type(req, "application/json");
        return httpd_resp_sendstr(req, "{\"status\":\"deleted\"}");
    }

    httpd_resp_send_404(req);
    return ESP_FAIL;
}

esp_err_t WebServer::handlePrint(httpd_req_t* req) {
    char buf[256] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON* root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    cJSON* jFile = cJSON_GetObjectItem(root, "filename");
    std::string filename = (jFile && jFile->valuestring) ? jFile->valuestring : "Unknown";
    cJSON_Delete(root);

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "🖨️  ĐANG XỬ LÝ LỆNH IN CHO TÀI LIỆU: [%s]", filename.c_str());
    ESP_LOGI(TAG, "=================================================");

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, "{\"status\":\"printing\"}");
}

} // namespace printer
