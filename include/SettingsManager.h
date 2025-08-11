#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include "RecordingService.h"
#include <string>
#include <map>

// 主题枚举
enum class Theme {
    LIGHT,  // 浅色主题
    DARK    // 深色主题
};

// 语言枚举
enum class Language {
    ENGLISH,  // 英语
    CHINESE   // 中文
};

// 热键结构
struct Hotkey {
    std::string name;        // 热键名称
    std::string description; // 热键描述
    std::string keySequence; // 按键序列
    bool enabled;            // 是否启用
};

// 应用设置结构
struct AppSettings {
    // UI设置
    Theme theme = Theme::DARK;           // 主题
    Language language = Language::CHINESE; // 语言
    
    // 录制默认设置
    RecConfig defaultConfig;             // 默认录制配置
    
    // 存储设置
    std::string savePath = "./recordings"; // 保存路径
    bool autoOrganize = true;              // 自动整理
    int keepDays = 30;                     // 保留天数
    
    // 高级设置
    bool enableHotkeys = true;             // 启用热键
    std::map<std::string, Hotkey> hotkeys; // 热键映射
};

/**
 * @brief 设置管理器
 */
class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();
    
    /**
     * @brief 加载设置
     * @return AppSettings 应用设置
     */
    AppSettings loadSettings();
    
    /**
     * @brief 保存设置
     * @param settings 应用设置
     * @return true 成功, false 失败
     */
    bool saveSettings(const AppSettings& settings);
    
    /**
     * @brief 迁移旧设置
     * @return true 成功, false 失败
     */
    bool migrateOldSettings();
    
    /**
     * @brief 获取默认设置
     * @return AppSettings 默认设置
     */
    AppSettings getDefaultSettings() const;
    
    /**
     * @brief 获取配置文件路径
     * @return 配置文件路径
     */
    std::string getConfigPath() const;
    
private:
    /**
     * @brief 创建默认热键配置
     * @return 热键映射表
     */
    std::map<std::string, Hotkey> createDefaultHotkeys() const;
    
    /**
     * @brief 安全保存设置到文件
     * @param settings 应用设置
     * @param filePath 文件路径
     * @return true 成功, false 失败
     */
    bool saveSettingsToFile(const AppSettings& settings, const std::string& filePath) const;
    
    /**
     * @brief 从文件加载设置
     * @param filePath 文件路径
     * @return AppSettings 应用设置
     */
    AppSettings loadSettingsFromFile(const std::string& filePath) const;
    
    /**
     * @brief 验证设置有效性
     * @param settings 应用设置
     * @return true 有效, false 无效
     */
    bool validateSettings(const AppSettings& settings) const;
    
    std::string configPath;
};

#endif // SETTINGS_MANAGER_H