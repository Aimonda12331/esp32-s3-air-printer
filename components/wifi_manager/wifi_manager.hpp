#pragma once

#include <string>
#include <vector>
#include <functional>
#include "esp_err.h"

namespace printer {

/**
 * @brief Trạng thái Wi-Fi để giao diện hiển thị
 */
enum class WifiState {
    DISCONNECTED,  // Chưa kết nối
    CONNECTING,    // Đang trong quá trình kết nối
    CONNECTED,     // Đã có kết nối và nhận được IP
    CONNECT_FAILED // Thất bại (sai pass hoặc không tìm thấy AP)
};

/**
 * @brief Thông tin AP quét được để đổ lên danh sách trên giao diện
 */
struct WifiApRecord {
    std::string ssid;
    int8_t rssi;       // Cường độ tín hiệu (dBm)
    bool isLocked;     // Mạng có mật khẩu hay không
};

class WifiManager {
public:
    using WifiEventCallback = std::function<void(WifiState state)>;

    WifiManager();
    ~WifiManager();

    static WifiManager& getInstance();

    /**
     * @brief Khởi tạo Wi-Fi ở chế độ chỉ làm trạm (Station)
     */
    esp_err_t init();

    /**
     * @brief Khởi tạo Wi-Fi ở chế độ AP + Station (Vừa phát AP vừa có thể kết nối mạng)
     * @param apSsid Tên WiFi AP phát ra (để trống sẽ tự lấy "AirPrinter-XXXX" theo MAC)
     * @param apPass Mật khẩu AP (để trống là mạng mở)
     */
    esp_err_t initApSta(const std::string& apSsid = "", const std::string& apPass = "");

    /**
     * @brief Lấy 4 ký tự cuối của MAC address (ví dụ: "819c")
     */
    std::string getMacSuffix();

    /**
     * @brief Kết nối tới mạng Wi-Fi
     * @param ssid Tên Wi-Fi
     * @param password Mật khẩu
     * @param timeoutMs Thời gian chờ tối đa
     */
    esp_err_t connect(const std::string& ssid, const std::string& password, uint32_t timeoutMs = 10000);

    /**
     * @brief Ngắt kết nối Wi-Fi
     */
    esp_err_t disconnect();

    /**
     * @brief Lưu thông tin Wi-Fi vào Flash (NVS) để tự kết nối khi khởi động
     */
    esp_err_t saveCredentials(const std::string& ssid, const std::string& password);

    /**
     * @brief Tải thông tin Wi-Fi đã lưu từ Flash (NVS)
     * @return true nếu đã từng lưu Wi-Fi
     */
    bool loadCredentials(std::string& ssid, std::string& password);

    /**
     * @brief Xóa thông tin Wi-Fi đã lưu trong Flash
     */
    esp_err_t clearCredentials();

    /**
     * @brief Quét danh sách Wi-Fi xung quanh (chế độ passive scan chống sụt áp)
     */
    std::vector<WifiApRecord> scanNetworks();

    /**
     * @brief Đăng ký hàm nhận sự kiện
     */
    void setEventCallback(WifiEventCallback callback);

    /**
     * @brief Lấy trạng thái hiện tại
     */
    WifiState getState() const { return state_; }

    /**
     * @brief Lấy địa chỉ IP dạng chuỗi
     */
    std::string getIpString() const { return ipAddress_; }

private:
    static void wifiEventHandler(void* arg, const char* eventBase, int32_t eventId, void* eventData);

    WifiState state_{WifiState::DISCONNECTED};
    std::string ipAddress_{"0.0.0.0"};
    WifiEventCallback eventCallback_{nullptr};
    void* eventGroupHandle_{nullptr};
};

} // namespace printer
