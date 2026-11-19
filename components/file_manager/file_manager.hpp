#pragma once

#include <string>
#include <vector>
#include <mutex>
#include "esp_err.h"

namespace printer {

/**
 * @brief Thông tin tài liệu trong hàng đợi in
 */
struct PrintDocument {
    std::string name;
    std::string sizeStr;
};

/**
 * @brief Module quản lý tệp tin và hàng đợi in ấn
 * Phụ trách lưu trữ, khôi phục từ Flash NVS, thêm, xóa và quản lý danh sách file
 */
class FileManager {
public:
    static FileManager& getInstance();

    /**
     * @brief Khởi tạo module và tải danh sách tệp đã lưu trong Flash NVS
     */
    esp_err_t init();

    /**
     * @brief Thêm hoặc cập nhật tài liệu vào hàng đợi
     */
    bool addFile(const std::string& name, const std::string& sizeStr);
    bool addFile(const std::string& name, size_t bytes);

    /**
     * @brief Xóa một tài liệu khỏi hàng đợi và cập nhật Flash NVS
     */
    bool removeFile(const std::string& name);

    /**
     * @brief Xóa sạch toàn bộ danh sách tài liệu trong RAM và Flash NVS
     */
    void clearAll();

    /**
     * @brief Lấy bản sao danh sách tài liệu hiện tại (thread-safe)
     */
    std::vector<PrintDocument> getFiles();

    /**
     * @brief Kiểm tra xem một file có tồn tại trong hàng đợi không
     */
    bool hasFile(const std::string& name);

    /**
     * @brief Lấy số lượng tài liệu hiện có trong hàng đợi
     */
    size_t getFileCount();

    /**
     * @brief Tiện ích định dạng số byte sang chuỗi thân thiện (B, KB, MB)
     */
    static std::string formatBytes(size_t bytes);

private:
    FileManager();
    ~FileManager() = default;

    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;

    void loadFromNvs();
    void saveToNvs();

    std::vector<PrintDocument> files_;
    std::mutex mutex_;
};

} // namespace printer
