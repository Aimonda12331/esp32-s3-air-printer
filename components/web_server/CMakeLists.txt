idf_component_register(SRCS "web_server.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_http_server mdns wifi_manager nvs_flash json console)
