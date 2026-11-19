#pragma once

#include <string>
#include <vector>
#include "esp_err.h"
#include "esp_http_server.h"

namespace printer {

struct PrintDocument {
    std::string name;
    std::string sizeStr;
};

class WebServer {
public:
    static WebServer& getInstance();

    esp_err_t start(uint16_t port = 80);
    esp_err_t stop();

    std::string getDeviceName();
    esp_err_t setDeviceName(const std::string& name);
    void updateMdns(const std::string& hostname);

private:
    WebServer() = default;
    ~WebServer();

    httpd_handle_t serverHandle_{nullptr};
    std::string deviceName_;
    std::vector<PrintDocument> uploadedFiles_;

    // Các hàm xử lý HTTP
    static esp_err_t handleIndexHtml(httpd_req_t* req);
    static esp_err_t handleWifiScan(httpd_req_t* req);
    static esp_err_t handleConfig(httpd_req_t* req);
    static esp_err_t handleStatus(httpd_req_t* req);
    static esp_err_t handleUpload(httpd_req_t* req);
    static esp_err_t handleFiles(httpd_req_t* req);
    static esp_err_t handlePrint(httpd_req_t* req);

    std::string loadDeviceNameFromNvs();
    esp_err_t saveDeviceNameToNvs(const std::string& name);
};

} // namespace printer
