#include "control_led.hpp"
#include "wifi_manager.hpp"
#include "web_server.hpp"
#include "file_manager.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

extern "C" void esp_brownout_disable(void);

using printer::LedController;
using printer::RgbColor;
using printer::LedPattern;
using printer::WifiManager;
using printer::WifiState;
using printer::WebServer;
using printer::FileManager;

static const char* TAG = "MainApp";

// Cấu hình chân phần cứng
static constexpr gpio_num_t STATUS_LED_GPIO = GPIO_NUM_48;
static constexpr gpio_num_t BOOT_BUTTON_GPIO = GPIO_NUM_0;

// Đối tượng điều khiển LED trạng thái (chân GPIO 48)
static LedController statusLed(STATUS_LED_GPIO);

/**
 * @brief Task giám sát nút BOOT (GPIO 0).
 * Nhấn giữ đủ 10 giây -> Đèn nhấp nháy đỏ liên tục -> Xóa sạch file và pass Wi-Fi -> Restart về AP.
 */
static void bootButtonTask(void* pvParameters) {
    gpio_config_t io_conf{};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    int holdTimeMs = 0;
    const int CHECK_INTERVAL_MS = 50;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(CHECK_INTERVAL_MS));

        // Nút BOOT đang được nhấn giữ (mức 0)
        if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
            holdTimeMs += CHECK_INTERVAL_MS;

            // Đạt mốc 10 giây (10,000 ms)
            if (holdTimeMs >= 10000) {
                ESP_LOGW(TAG, "=========================================================");
                ESP_LOGW(TAG, "🚨 NÚT BOOT ĐÃ GIỮ ĐỦ 10 GIÂY -> TIẾN HÀNH FACTORY RESET!");
                ESP_LOGW(TAG, "=========================================================");

                // 1. Nhấp nháy đỏ liên tục báo hiệu reset
                statusLed.setPattern(LedPattern::BLINK_FAST, RgbColor::Red());

                // 2. Xóa danh sách file trong hàng đợi qua FileManager
                FileManager::getInstance().clearAll();

                // 3. Xóa sạch bộ nhớ Flash NVS (xóa Wi-Fi, pass, tên máy in mDNS...)
                ESP_LOGI(TAG, "Đang xóa sạch toàn bộ Flash NVS...");
                nvs_flash_erase();

                // Giữ đèn nhấp nháy đỏ liên tục trong 2.5 giây để người dùng nhìn thấy rõ ràng
                vTaskDelay(pdMS_TO_TICKS(2500));

                // 4. Khởi động lại hệ thống về trạng thái AP ban đầu
                ESP_LOGW(TAG, "Đang khởi động lại hệ thống về chế độ AP ban đầu...");
                esp_restart();
            }
        } else {
            // Nút được nhả ra trước khi đạt 10 giây
            if (holdTimeMs > 0 && holdTimeMs < 10000) {
                ESP_LOGI(TAG, "Nút BOOT nhả sớm (%d ms) -> Hủy thao tác Reset.", holdTimeMs);
            }
            holdTimeMs = 0;
        }
    }
}

extern "C" void app_main(void) {
    // 0. Tắt Brownout Detector ngay lập tức để chống sụt áp nguồn USB khi RF khởi động
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    esp_brownout_disable();

    // Ẩn các cảnh báo ngắt socket bình thường (mã 104) từ trình duyệt
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);

    // 1. Khởi tạo LED trạng thái
    ESP_ERROR_CHECK(statusLed.begin());
    // Bật hiệu ứng thở màu Xanh Dương báo hiệu đang ở chế độ AP chờ cấu hình
    statusLed.setPattern(LedPattern::BREATHE, RgbColor::Blue());

    auto& wifi = WifiManager::getInstance();

    // 2. Đăng ký nhận sự kiện đổi trạng thái Wi-Fi để cập nhật đèn LED
    wifi.setEventCallback([](WifiState state) {
        switch (state) {
            case WifiState::CONNECTING:
                ESP_LOGI(TAG, "Trạng thái: ĐANG KẾT NỐI -> Nháy vàng nhanh");
                statusLed.setPattern(LedPattern::BLINK_FAST, RgbColor::Yellow());
                break;

            case WifiState::CONNECTED:
                ESP_LOGI(TAG, "Trạng thái: ĐÃ KẾT NỐI -> Xanh lá cố định (IP: %s)", 
                         WifiManager::getInstance().getIpString().c_str());
                statusLed.setPattern(LedPattern::SOLID, RgbColor::Green());
                break;

            case WifiState::CONNECT_FAILED:
                ESP_LOGW(TAG, "Trạng thái: KẾT NỐI THẤT BẠI -> Nháy đỏ chậm");
                statusLed.setPattern(LedPattern::BLINK_SLOW, RgbColor::Red());
                break;

            case WifiState::DISCONNECTED:
                ESP_LOGI(TAG, "Trạng thái: CHỜ CẤU HÌNH -> Thở xanh dương");
                statusLed.setPattern(LedPattern::BREATHE, RgbColor::Blue());
                break;
        }
    });

    // 3. Khởi tạo Wi-Fi ở chế độ AP+STA:
    // Tự động phát AP có tên dạng "AirPrinter-xxxx" (theo 4 số cuối MAC), không mật khẩu
    ESP_ERROR_CHECK(wifi.initApSta());

    // 4. Khởi chạy Web Server & mDNS
    auto& webServer = WebServer::getInstance();
    ESP_ERROR_CHECK(webServer.start(80));

    // 5. Kiểm tra Flash: Tự động kết nối lại nếu đã từng lưu Wi-Fi
    std::string savedSsid, savedPass;
    if (wifi.loadCredentials(savedSsid, savedPass)) {
        ESP_LOGI(TAG, "Đã tìm thấy Wi-Fi trong Flash: SSID=[%s]. Bắt đầu tự động kết nối...", savedSsid.c_str());
        xTaskCreate([](void*) {
            vTaskDelay(pdMS_TO_TICKS(800)); // Đợi hệ thống mạng sẵn sàng
            std::string s, p;
            auto& w = WifiManager::getInstance();
            if (w.loadCredentials(s, p)) {
                w.connect(s, p, 15000);
            }
            vTaskDelete(nullptr);
        }, "wifi_autoconn", 4096, nullptr, 5, nullptr);
    } else {
        ESP_LOGI(TAG, "Chưa lưu Wi-Fi nào trong Flash. Đang chờ cấu hình qua AP...");
    }

    // 6. Khởi chạy Task giám sát nút BOOT để Factory Reset (giữ 10s)
    xTaskCreate(bootButtonTask, "boot_btn_task", 3072, nullptr, 10, nullptr);

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  HỆ THỐNG AIR PRINTER ĐÃ SẴN SÀNG!");
    ESP_LOGI(TAG, "  1. Wi-Fi AP: AirPrinter-%s (IP: 192.168.4.1)", wifi.getMacSuffix().c_str());
    ESP_LOGI(TAG, "  2. Tên miền mDNS: http://%s.local", webServer.getDeviceName().c_str());
    ESP_LOGI(TAG, "  3. Giữ nút BOOT 10s để Factory Reset về AP ban đầu.");
    ESP_LOGI(TAG, "=================================================");

    // app_main duy trì để hệ thống hoạt động
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
