#include "control_led.hpp"
#include "wifi_manager.hpp"
#include "web_server.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using printer::LedController;
using printer::RgbColor;
using printer::LedPattern;
using printer::WifiManager;
using printer::WifiState;
using printer::WebServer;

static const char* TAG = "MainApp";

// Đối tượng điều khiển LED trạng thái (chân GPIO 48)
static LedController statusLed(GPIO_NUM_48);

extern "C" void app_main(void) {
    // 1. Khởi tạo LED trạng thái
    statusLed.begin();
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

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  HỆ THỐNG CẤU HÌNH AIR PRINTER ĐÃ SẴN SÀNG!");
    ESP_LOGI(TAG, "  1. Kết nối vào Wi-Fi: AirPrinter-%s", wifi.getMacSuffix().c_str());
    ESP_LOGI(TAG, "  2. Mở trình duyệt gõ: http://192.168.4.1");
    ESP_LOGI(TAG, "  3. Tên miền mDNS hiện tại: http://%s.local", webServer.getDeviceName().c_str());
    ESP_LOGI(TAG, "=================================================");

    // app_main duy trì để hệ thống hoạt động
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
