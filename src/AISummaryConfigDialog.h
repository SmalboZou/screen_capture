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

struct AISummaryConfig {
    QString provider;        // 模型提供商
    QString baseUrl;         // API Base URL
    QString apiKey;          // API Key
    QString modelName;       // 模型名称
    bool enabled;            // 是否启用
    
    // 默认构造函数
    AISummaryConfig() : enabled(false) {}
    
    // 判断配置是否有效
    bool isValid() const {
        return !provider.isEmpty() && !baseUrl.isEmpty() && 
               !apiKey.isEmpty() && !modelName.isEmpty();
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
    
private:
    void setupUI();
    void updateModelOptions();
    void populateDefaultSettings();
    QString getDefaultBaseUrl(const QString& provider) const;
    QStringList getDefaultModels(const QString& provider) const;
    
    // UI组件
    QComboBox *providerCombo;
    QLineEdit *baseUrlEdit;
    QLineEdit *apiKeyEdit;
    QComboBox *modelCombo;
    QPushButton *testButton;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    
    // 网络管理
    QNetworkAccessManager *networkManager;
    QNetworkReply *currentReply;
};

#endif // AISUMMARYCONFIGDIALOG_H
