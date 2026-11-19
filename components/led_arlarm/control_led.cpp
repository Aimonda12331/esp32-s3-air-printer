#include "control_led.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace printer {

static const char* TAG = "LedController";

LedController::LedController(gpio_num_t gpio, uint32_t ledCount)
    : gpio_(gpio), ledCount_(ledCount), strip_(nullptr) {
}

LedController::~LedController() {
    if (taskHandle_ != nullptr) {
        vTaskDelete(taskHandle_);
        taskHandle_ = nullptr;
    }
    if (strip_ != nullptr) {
        led_strip_del(strip_);
        strip_ = nullptr;
    }
    
}

// Move constructor
LedController::LedController(LedController&& other) noexcept
    : gpio_(other.gpio_), ledCount_(other.ledCount_), strip_(other.strip_) {
    other.strip_ = nullptr;
}

// Move assignment
LedController& LedController::operator=(LedController&& other) noexcept {
    if (this != &other) {
        if (strip_ != nullptr) {
            led_strip_del(strip_);
        }
        gpio_ = other.gpio_;
        ledCount_ = other.ledCount_;
        strip_ = other.strip_;
        other.strip_ = nullptr;
    }
    return *this;
}

esp_err_t LedController::begin() {
    led_strip_config_t stripConfig = {
        .strip_gpio_num = gpio_,
        .max_leds = ledCount_,
    };

    led_strip_rmt_config_t rmtConfig = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };

    esp_err_t err = led_strip_new_rmt_device(&stripConfig, &rmtConfig, &strip_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Khởi tạo led_strip RMT thất bại trên GPIO %d (lỗi: %s)", gpio_, esp_err_to_name(err));
        return err;
    }

    return turnOff();
}

esp_err_t LedController::setColor(uint32_t index, RgbColor color) {
    return setColor(index, color.r, color.g, color.b);
}

esp_err_t LedController::setColor(uint32_t index, uint8_t red, uint8_t green, uint8_t blue) {
    if (strip_ == nullptr) {
        ESP_LOGE(TAG, "Chưa gọi begin() trước khi dùng!");
        return ESP_ERR_INVALID_STATE;
    }
    if (index >= ledCount_) {
        ESP_LOGE(TAG, "Index %lu vượt quá số LED (%lu)", (unsigned long)index, (unsigned long)ledCount_);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = led_strip_set_pixel(strip_, index, red, green, blue);
    if (err != ESP_OK) return err;

    return led_strip_refresh(strip_);
}

esp_err_t LedController::setAll(RgbColor color) {
    return setAll(color.r, color.g, color.b);
}

esp_err_t LedController::setAll(uint8_t red, uint8_t green, uint8_t blue) {
    if (strip_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    for (uint32_t i = 0; i < ledCount_; ++i) {
        esp_err_t err = led_strip_set_pixel(strip_, i, red, green, blue);
        if (err != ESP_OK) return err;
    }

    return led_strip_refresh(strip_);
}

esp_err_t LedController::turnOff() {
    if (strip_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    return led_strip_clear(strip_);
}

void LedController::blink(uint32_t delayMs, RgbColor color) {
    setAll(color);
    vTaskDelay(pdMS_TO_TICKS(delayMs));

    turnOff();
    vTaskDelay(pdMS_TO_TICKS(delayMs));
}

void LedController::setPattern(LedPattern pattern, RgbColor color) {
    currentPattern_ = pattern;
    currentColor_ = color;

    // Khởi tạo task chạy ngầm nếu chưa có
    if (taskHandle_ == nullptr) {
        xTaskCreate(&LedController::ledTask, "led_task", 2048, this, 2, &taskHandle_);
    }
}

void LedController::ledTask(void* arg) {
    auto* self = static_cast<LedController*>(arg);

    while (true) {
        switch (self->currentPattern_) {
            case LedPattern::OFF:
                self->turnOff();
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LedPattern::SOLID:
                self->setAll(self->currentColor_);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LedPattern::BLINK_SLOW:
                self->setAll(self->currentColor_);
                vTaskDelay(pdMS_TO_TICKS(1000));
                self->turnOff();
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;

            case LedPattern::BLINK_FAST:
                self->setAll(self->currentColor_);
                vTaskDelay(pdMS_TO_TICKS(200));
                self->turnOff();
                vTaskDelay(pdMS_TO_TICKS(200));
                break;

            case LedPattern::BREATHE: { // Hiệu ứng thở (sáng dần rồi tối dần)
                for (int b = 5; b <= 100; b += 5) {
                    if (self->currentPattern_ != LedPattern::BREATHE) break;
                    RgbColor c{
                        static_cast<uint8_t>(self->currentColor_.r * b / 100),
                        static_cast<uint8_t>(self->currentColor_.g * b / 100),
                        static_cast<uint8_t>(self->currentColor_.b * b / 100)
                    };
                    self->setAll(c);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                for (int b = 100; b >= 5; b -= 5) {
                    if (self->currentPattern_ != LedPattern::BREATHE) break;
                    RgbColor c{
                        static_cast<uint8_t>(self->currentColor_.r * b / 100),
                        static_cast<uint8_t>(self->currentColor_.g * b / 100),
                        static_cast<uint8_t>(self->currentColor_.b * b / 100)
                    };
                    self->setAll(c);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                break;
            }
        }
    }
}

} // namespace printer