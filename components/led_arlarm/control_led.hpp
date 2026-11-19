#pragma once

#include <cstdint>
#include "driver/gpio.h"
#include "esp_err.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace printer {

enum class LedPattern {
    OFF,            // Tắt
    SOLID,          // Sáng cố định
    BLINK_SLOW,     // Nháy chậm (1000ms) - VD: Standby / Đang kết nối
    BLINK_FAST,     // Nháy nhanh (200ms)  - VD: Lỗi / Kẹt giấy
    BREATHE         // Hiệu ứng thở (nếu muốn làm đẹp)
};

/**
 * @brief Cấu trúc biểu diễn màu sắc RGB
 */
struct RgbColor {
    uint8_t r{0};
    uint8_t g{0};
    uint8_t b{0};

    // Một số màu sắc thường dùng định nghĩa sẵn
    static constexpr RgbColor Red()    { return {50, 0, 0}; }
    static constexpr RgbColor Green()  { return {0, 50, 0}; }
    static constexpr RgbColor Blue()   { return {0, 0, 50}; }
    static constexpr RgbColor Yellow() { return {50, 50, 0}; }
    static constexpr RgbColor White()  { return {50, 50, 50}; }
    static constexpr RgbColor Off()    { return {0, 0, 0}; }
};

/**
 * @class LedController
 * @brief Lớp điều khiển LED RGB địa chỉ (WS2812/SK6812) trên ESP32.
 */
class LedController {
public:
    /**
     * @brief Constructor (explicit để chống ép kiểu ngầm định)
     */
    explicit LedController(gpio_num_t gpio, uint32_t ledCount = 1);

    /**
     * @brief Destructor giải phóng tài nguyên phần cứng
     */
    ~LedController();

    // =========================================================================
    // Rule of Five: Cấm copy để tránh double-free con trỏ strip_
    // =========================================================================
    LedController(const LedController&) = delete;
    LedController& operator=(const LedController&) = delete;

    // Cho phép Move (chuyển giao quyền sở hữu)
    LedController(LedController&& other) noexcept;
    LedController& operator=(LedController&& other) noexcept;

    /**
     * @brief Khởi tạo phần cứng RMT driver cho LED strip
     * @return ESP_OK nếu thành công, mã lỗi nếu thất bại
     */
    [[nodiscard]] esp_err_t begin();

    /**
     * @brief Đặt màu cho 1 LED tại vị trí index
     */
    esp_err_t setColor(uint32_t index, RgbColor color);
    esp_err_t setColor(uint32_t index, uint8_t red, uint8_t green, uint8_t blue);

    /**
     * @brief Đặt màu cho TOÀN BỘ dải LED
     */
    esp_err_t setAll(RgbColor color);
    esp_err_t setAll(uint8_t red, uint8_t green, uint8_t blue);

    /**
     * @brief Tắt toàn bộ LED
     */
    esp_err_t turnOff();

    /**
     * @brief Nhấp nháy toàn bộ LED (hàm dạng blocking)
     */
    void blink(uint32_t delayMs, RgbColor color = RgbColor::White());

    /**
     * @brief Kiểm tra xem driver đã được khởi tạo thành công chưa
     */
    [[nodiscard]] bool isInitialized() const { return strip_ != nullptr; }

    /**
     * @brief API cao cấp: Đặt chế độ hiệu ứng chạy nền (Non-blocking)
     * Hàm này trả về ngay lập tức, LED sẽ tự nhấp nháy trong tác vụ nền.
     */
    void setPattern(LedPattern pattern, RgbColor color);

private:
    gpio_num_t gpio_;
    uint32_t ledCount_;
    led_strip_handle_t strip_{nullptr};
    static void ledTask(void* arg); // FreeRTOS Task chạy ngầm
    LedPattern currentPattern_{LedPattern::OFF};
    RgbColor currentColor_{RgbColor::Off()};
    TaskHandle_t taskHandle_{nullptr};
};

} // namespace printer