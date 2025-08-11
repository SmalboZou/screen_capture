#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "DataTypes.h"
#include <string>
#include <vector>
#include <ctime>

// 录制文件信息结构
struct RecordingInfo {
    std::string path;           // 文件路径
    std::string name;           // 文件名
    uint64_t size;              // 文件大小(bytes)
    std::time_t creationTime;   // 创建时间
    int duration;               // 持续时间(秒)
    std::string format;         // 文件格式
};

// 导出格式枚举
enum class ExportFormat {
    ZIP,        // ZIP压缩包
    TAR_GZ,     // TAR.GZ压缩包
    FOLDER      // 文件夹
};

/**
 * @brief 文件管理器
 */
class FileManager {
public:
    FileManager();
    ~FileManager();
    
    /**
     * @brief 整理录制文件
     * @param basePath 基础路径
     */
    void organizeRecordings(const std::string& basePath = "./recordings");
    
    /**
     * @brief 清理旧文件
     * @param days 保留天数
     * @param basePath 基础路径
     */
    void cleanOldFiles(int days, const std::string& basePath = "./recordings");
    
    /**
     * @brief 导出项目
     * @param format 导出格式
     * @param outputPath 输出路径
     * @param basePath 基础路径
     * @return true 成功, false 失败
     */
    bool exportProject(ExportFormat format, const std::string& outputPath, 
                      const std::string& basePath = "./recordings");
    
    /**
     * @brief 获取所有录制文件信息
     * @param basePath 基础路径
     * @return 录制文件信息列表
     */
    std::vector<RecordingInfo> getAllRecordings(const std::string& basePath = "./recordings") const;
    
    /**
     * @brief 删除文件
     * @param filePath 文件路径
     * @return true 成功, false 失败
     */
    bool deleteFile(const std::string& filePath);
    
    /**
     * @brief 获取文件信息
     * @param filePath 文件路径
     * @return 录制文件信息
     */
    RecordingInfo getFileInfo(const std::string& filePath) const;
    
private:
    /**
     * @brief 获取日期路径
     * @param time 时间
     * @return 日期路径
     */
    std::string getDatePath(std::time_t time) const;
    
    /**
     * @brief 移动文件
     * @param srcPath 源路径
     * @param dstPath 目标路径
     * @return true 成功, false 失败
     */
    bool moveFile(const std::string& srcPath, const std::string& dstPath);
    
    /**
     * @brief 复制文件
     * @param srcPath 源路径
     * @param dstPath 目标路径
     * @return true 成功, false 失败
     */
    bool copyFile(const std::string& srcPath, const std::string& dstPath);
    
    /**
     * @brief 创建目录
     * @param path 目录路径
     * @return true 成功, false 失败
     */
    bool createDirectory(const std::string& path) const;
    
    /**
     * @brief 获取文件大小
     * @param filePath 文件路径
     * @return 文件大小(bytes)
     */
    uint64_t getFileSize(const std::string& filePath) const;
    
    /**
     * @brief 解析文件格式
     * @param fileName 文件名
     * @return 文件格式字符串
     */
    std::string parseFileFormat(const std::string& fileName) const;
};

#endif // FILE_MANAGER_H