#ifndef AISUMMARYCONFIGDIALOG_H
#define AISUMMARYCONFIGDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

struct AISummaryConfig {
    QString provider;        // 模型提供商
    QString baseUrl;         // API Base URL
    QString apiKey;          // API Key
    QString visionModelName; // 视觉模型名称（用于图像分析）
    QString summaryModelName;// 总结模型名称（用于文本总结）
    bool enabled;            // 是否启用
    
    // 为了兼容性，保留原有的modelName属性（作为visionModelName的别名）
    QString modelName;       // 兼容性属性，映射到visionModelName
    
    // 默认构造函数
    AISummaryConfig() : enabled(false) {}
    
    // 判断配置是否有效
    bool isValid() const {
        return !provider.isEmpty() && !baseUrl.isEmpty() && 
               !apiKey.isEmpty() && !visionModelName.isEmpty() && !summaryModelName.isEmpty();
    }
};

class AISummaryConfigDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit AISummaryConfigDialog(QWidget *parent = nullptr);
    ~AISummaryConfigDialog();
    
    // 获取和设置配置
    AISummaryConfig getConfig() const;
    void setConfig(const AISummaryConfig& config);

private slots:
    void onProviderChanged(const QString& provider);
    void onTestConnection();
    void onNetworkReplyFinished();
    void onModelListReplyFinished();
    void onRefreshModelsClicked();
    
private:
    void setupUI();
    void updateModelOptions();
    void populateDefaultSettings();
    void fetchAvailableModels();
    void parseModelListResponse(const QByteArray& data);
    bool isVisionModel(const QString& modelName) const;
    bool isSummaryModel(const QString& modelName) const;
    QString getDefaultBaseUrl(const QString& provider) const;
    QStringList getDefaultVisionModels(const QString& provider) const;
    QStringList getDefaultSummaryModels(const QString& provider) const;
    QStringList getDefaultModels(const QString& provider) const; // 兼容性方法
    
    // UI组件
    QComboBox *providerCombo;
    QLineEdit *baseUrlEdit;
    QLineEdit *apiKeyEdit;
    QComboBox *visionModelCombo;      // 视觉模型选择
    QComboBox *summaryModelCombo;     // 总结模型选择
    QPushButton *refreshModelsButton;
    QPushButton *testButton;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QLabel *visionModelStatusLabel;   // 视觉模型状态
    QLabel *summaryModelStatusLabel;  // 总结模型状态
    
    // 网络管理
    QNetworkAccessManager *networkManager;
    QNetworkReply *currentReply;
    QNetworkReply *modelListReply;
};

#endif // AISUMMARYCONFIGDIALOG_H
