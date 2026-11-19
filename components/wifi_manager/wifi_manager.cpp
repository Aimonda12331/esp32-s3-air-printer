#include "wifi_manager.hpp"

#include <cstring>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

namespace printer {

static const char* TAG = "WifiManager";

// Các bit cờ trạng thái kết nối
static constexpr int WIFI_CONNECTED_BIT = BIT0;
static constexpr int WIFI_FAIL_BIT      = BIT1;

WifiManager::WifiManager() {
    eventGroupHandle_ = xEventGroupCreate();
}

WifiManager::~WifiManager() {
    if (eventGroupHandle_) {
        vEventGroupDelete(reinterpret_cast<EventGroupHandle_t>(eventGroupHandle_));
    }
}

WifiManager& WifiManager::getInstance() {
    static WifiManager instance;
    return instance;
}

std::string WifiManager::getMacSuffix() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02x%02x", mac[4], mac[5]);
    return std::string(buf);
}

esp_err_t WifiManager::init() {
    // 1. Khởi tạo NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) return ret;

    // 2. Khởi tạo Netif và Event Loop mặc định
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // 3. Khởi tạo driver WiFi ở chế độ trạm (Station)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 4. Đăng ký lắng nghe sự kiện
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::wifiEventHandler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::wifiEventHandler, this, nullptr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Giảm công suất phát để chống sụt áp nguồn (14 dBm = 56 x 0.25 dBm)
    esp_wifi_set_max_tx_power(56);

    ESP_LOGI(TAG, "WifiManager khởi tạo chế độ STA thành công.");
    return ESP_OK;
}

esp_err_t WifiManager::initApSta(const std::string& apSsid, const std::string& apPass) {
    // 1. Khởi tạo NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) return ret;

    // 2. Khởi tạo Netif cho cả STA và AP
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    // 3. Khởi tạo driver WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 4. Đăng ký sự kiện
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::wifiEventHandler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::wifiEventHandler, this, nullptr));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // 5. Cấu hình AP (Mặc định lấy theo MAC nếu không truyền tên)
    std::string actualSsid = apSsid.empty() ? ("AirPrinter-" + getMacSuffix()) : apSsid;
    wifi_config_t apConfig{};
    std::strncpy(reinterpret_cast<char*>(apConfig.ap.ssid), actualSsid.c_str(), sizeof(apConfig.ap.ssid));
    apConfig.ap.ssid_len = actualSsid.length();
    apConfig.ap.channel = 1;
    apConfig.ap.max_connection = 4;
    if (apPass.empty()) {
        apConfig.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        std::strncpy(reinterpret_cast<char*>(apConfig.ap.password), apPass.c_str(), sizeof(apConfig.ap.password));
        apConfig.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &apConfig));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Giảm công suất phát để chống sụt áp nguồn (14 dBm = 56 x 0.25 dBm)
    esp_wifi_set_max_tx_power(56);

    ESP_LOGI(TAG, "Đã khởi chạy AP: [%s] (IP AP: 192.168.4.1)", actualSsid.c_str());
    return ESP_OK;
}

esp_err_t WifiManager::connect(const std::string& ssid, const std::string& password, uint32_t timeoutMs) {
    wifi_config_t wifiConfig{};
    std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.ssid), ssid.c_str(), sizeof(wifiConfig.sta.ssid));
    std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.password), password.c_str(), sizeof(wifiConfig.sta.password));
    wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    state_ = WifiState::CONNECTING;
    if (eventCallback_) eventCallback_(state_);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "Đang kết nối tới: %s...", ssid.c_str());

    if (timeoutMs > 0) {
        auto eventGroup = reinterpret_cast<EventGroupHandle_t>(eventGroupHandle_);
        EventBits_t bits = xEventGroupWaitBits(
            eventGroup,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(timeoutMs));

        if (bits & WIFI_CONNECTED_BIT) {
            return ESP_OK;
        } else {
            state_ = WifiState::CONNECT_FAILED;
            if (eventCallback_) eventCallback_(state_);
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

esp_err_t WifiManager::disconnect() {
    state_ = WifiState::DISCONNECTED;
    ipAddress_ = "0.0.0.0";
    if (eventCallback_) eventCallback_(state_);
    return esp_wifi_disconnect();
}

esp_err_t WifiManager::saveCredentials(const std::string& ssid, const std::string& password) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi_cfg", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    nvs_set_str(handle, "ssid", ssid.c_str());
    nvs_set_str(handle, "pass", password.c_str());
    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG, "Đã lưu Wi-Fi vào Flash NVS: SSID=[%s]", ssid.c_str());
    return ESP_OK;
}

bool WifiManager::loadCredentials(std::string& ssid, std::string& password) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi_cfg", NVS_READONLY, &handle);
    if (err != ESP_OK) return false;

    char sBuf[64] = {0};
    char pBuf[64] = {0};
    size_t sLen = sizeof(sBuf);
    size_t pLen = sizeof(pBuf);

    esp_err_t err1 = nvs_get_str(handle, "ssid", sBuf, &sLen);
    esp_err_t err2 = nvs_get_str(handle, "pass", pBuf, &pLen);
    nvs_close(handle);

    if (err1 == ESP_OK && sLen > 1) {
        ssid = sBuf;
        password = (err2 == ESP_OK) ? pBuf : "";
        return true;
    }
    return false;
}

esp_err_t WifiManager::clearCredentials() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wifi_cfg", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    nvs_erase_all(handle);
    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG, "Đã xóa Wi-Fi khỏi Flash NVS.");
    return ESP_OK;
}

std::vector<WifiApRecord> WifiManager::scanNetworks() {
    std::vector<WifiApRecord> apList;

    wifi_scan_config_t scanConfig{};
    scanConfig.show_hidden = false;
    scanConfig.scan_type = WIFI_SCAN_TYPE_PASSIVE; // Quét thụ động chống sụt áp
    scanConfig.scan_time.passive = 120;

    ESP_LOGI(TAG, "Bắt đầu quét mạng WiFi...");
    if (esp_wifi_scan_start(&scanConfig, true) != ESP_OK) {
        return apList;
    }

    uint16_t number = 20;
    std::vector<wifi_ap_record_t> rawAps(number);
    esp_wifi_scan_get_ap_records(&number, rawAps.data());

    for (uint16_t i = 0; i < number; ++i) {
        WifiApRecord rec;
        rec.ssid = reinterpret_cast<char*>(rawAps[i].ssid);
        rec.rssi = rawAps[i].rssi;
        rec.isLocked = (rawAps[i].authmode != WIFI_AUTH_OPEN);
        apList.push_back(rec);
    }

    ESP_LOGI(TAG, "Quét thấy %zu mạng.", apList.size());
    return apList;
}

void WifiManager::setEventCallback(WifiEventCallback callback) {
    eventCallback_ = std::move(callback);
}

void WifiManager::wifiEventHandler(void* arg, const char* eventBase, int32_t eventId, void* eventData) {
    auto* self = static_cast<WifiManager*>(arg);
    auto eventGroup = reinterpret_cast<EventGroupHandle_t>(self->eventGroupHandle_);

    if (eventBase == WIFI_EVENT) {
        switch (eventId) {
            case WIFI_EVENT_STA_START:
                break;
            case WIFI_EVENT_STA_DISCONNECTED: {
                self->state_ = WifiState::DISCONNECTED;
                self->ipAddress_ = "0.0.0.0";
                xEventGroupSetBits(eventGroup, WIFI_FAIL_BIT);

                // Mất kết nối STA -> Tự động bật lại chế độ AP để người dùng có thể cấu hình lại
                wifi_mode_t curMode;
                if (esp_wifi_get_mode(&curMode) == ESP_OK && curMode == WIFI_MODE_STA) {
                    ESP_LOGW(TAG, "Mất kết nối Wi-Fi nhà -> TỰ ĐỘNG BẬT LẠI AP để người dùng kết nối...");
                    esp_wifi_set_mode(WIFI_MODE_APSTA);
                }

                if (self->eventCallback_) self->eventCallback_(self->state_);
                ESP_LOGW(TAG, "Đã ngắt kết nối WiFi STA.");
                break;
            }
            default:
                break;
        }
    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(eventData);
        char ipStr[16];
        esp_ip4addr_ntoa(&event->ip_info.ip, ipStr, sizeof(ipStr));

        self->ipAddress_ = ipStr;
        self->state_ = WifiState::CONNECTED;
        xEventGroupSetBits(eventGroup, WIFI_CONNECTED_BIT);

        ESP_LOGI(TAG, "Đã nhận IP STA: %s", self->ipAddress_.c_str());

        // Sau khi vào được STA -> TẮT CHẾ ĐỘ AP để tiết kiệm năng lượng, chống sụt áp và giải phóng tài nguyên RF
        wifi_mode_t curMode;
        if (esp_wifi_get_mode(&curMode) == ESP_OK && curMode != WIFI_MODE_STA) {
            ESP_LOGI(TAG, "=========================================================");
            ESP_LOGI(TAG, "📶 ĐÃ KẾT NỐI VÀO STA -> TIẾN HÀNH TẮT ĐIỂM PHÁT AP!");
            ESP_LOGI(TAG, "=========================================================");
            esp_wifi_set_mode(WIFI_MODE_STA);
        }

        if (self->eventCallback_) self->eventCallback_(self->state_);
    }
}

} // namespace printer
