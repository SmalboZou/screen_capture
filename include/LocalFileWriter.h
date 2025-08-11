#ifndef LOCAL_FILE_WRITER_H
#define LOCAL_FILE_WRITER_H

#include "DataTypes.h"
#include <string>
#include <mutex>
#include <cstdint>

// 前向声明
typedef void* FileHandle;

/**
 * @brief 本地文件写入器
 */
class LocalFileWriter {
public:
    LocalFileWriter();
    ~LocalFileWriter();
    
    /**
     * @brief 打开文件
     * @param path 文件路径
     * @param format 文件格式
     * @return true 成功, false 失败
     */
    bool open(const std::string& path, FileFormat format);
    
    /**
     * @brief 写入媒体包
     * @param packet 媒体包
     * @return true 成功, false 失败
     */
    bool writePacket(const MediaPacket& packet);
    
    /**
     * @brief 完成写入并关闭文件
     * @return true 成功, false 失败
     */
    bool finalize();
    
    /**
     * @brief 获取已写字节数
     * @return 已写字节数
     */
    uint64_t getBytesWritten() const;
    
private:
    /**
     * @brief 判断是否需要分割文件
     * @return true 需要分割, false 不需要
     */
    bool shouldSplitFile() const;
    
    /**
     * @brief 创建新的文件段
     * @return true 成功, false 失败
     */
    bool createNewSegment();
    
    FileHandle fileHandle;
    std::mutex writeMutex;
    uint64_t bytesWritten;
    FileFormat currentFormat;
    std::string basePath;
    int segmentIndex;
    static const uint64_t splitThreshold = 1024 * 1024 * 500; // 500MB
};

#endif // LOCAL_FILE_WRITER_H