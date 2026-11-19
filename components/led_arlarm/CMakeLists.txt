idf_component_register(SRCS "control_led.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES espressif__led_strip driver)