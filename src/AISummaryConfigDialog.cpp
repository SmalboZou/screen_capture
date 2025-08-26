#include "AISummaryConfigDialog.h"
#include <QApplication>
#include <QTimer>

AISummaryConfigDialog::AISummaryConfigDialog(QWidget *parent)
    : QDialog(parent)
    , networkManager(new QNetworkAccessManager(this))
    , currentReply(nullptr)
{
    setWindowTitle("AI视频内容总结配置");
    setModal(true);
    setMinimumSize(500, 400);
    resize(600, 450);
    
    setupUI();
    
    // 连接信号
    connect(providerCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &AISummaryConfigDialog::onProviderChanged);
    connect(testButton, &QPushButton::clicked, this, &AISummaryConfigDialog::onTestConnection);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // 初始化默认设置
    onProviderChanged(providerCombo->currentText());
}

AISummaryConfigDialog::~AISummaryConfigDialog() {
    if (currentReply) {
        currentReply->abort();
        currentReply->deleteLater();
    }
}

void AISummaryConfigDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 说明文本
    QLabel *descLabel = new QLabel(
        "配置AI模型用于自动生成录制视频的内容总结。\n"
        "系统将从录制的视频中提取关键帧，使用视觉模型分析每帧内容，"
        "然后生成完整的视频内容总结。"
    );
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("QLabel { padding: 10px; background-color: #f0f8ff; border-radius: 5px; }");
    mainLayout->addWidget(descLabel);
    
    // 配置表单组
    QGroupBox *configGroup = new QGroupBox("模型配置");
    QFormLayout *formLayout = new QFormLayout(configGroup);
    
    // 模型提供商选择
    providerCombo = new QComboBox();
    providerCombo->addItems({
        "OpenAI",
        "硅基流动 (SiliconFlow)", 
        "智谱AI (GLM)",
        "月之暗面 (Kimi)",
        "自定义"
    });
    formLayout->addRow("模型提供商:", providerCombo);
    
    // API Base URL
    baseUrlEdit = new QLineEdit();
    baseUrlEdit->setPlaceholderText("例如: https://api.openai.com/v1");
    formLayout->addRow("API Base URL:", baseUrlEdit);
    
    // API Key
    apiKeyEdit = new QLineEdit();
    apiKeyEdit->setEchoMode(QLineEdit::Password);
    apiKeyEdit->setPlaceholderText("请输入您的API密钥");
    formLayout->addRow("API Key:", apiKeyEdit);
    
    // 模型名称
    modelCombo = new QComboBox();
    modelCombo->setEditable(true);
    formLayout->addRow("模型名称:", modelCombo);
    
    mainLayout->addWidget(configGroup);
    
    // 测试连接区域
    QGroupBox *testGroup = new QGroupBox("连接测试");
    QVBoxLayout *testLayout = new QVBoxLayout(testGroup);
    
    QHBoxLayout *testButtonLayout = new QHBoxLayout();
    testButton = new QPushButton("测试连接");
    testButton->setStyleSheet(
        "QPushButton { background-color: #007bff; color: white; font-weight: bold; "
        "padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #0056b3; }"
        "QPushButton:disabled { background-color: #6c757d; }"
    );
    testButtonLayout->addWidget(testButton);
    testButtonLayout->addStretch();
    
    progressBar = new QProgressBar();
    progressBar->setVisible(false);
    progressBar->setRange(0, 0); // 无限进度条
    
    statusLabel = new QLabel("请配置模型参数后测试连接");
    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("QLabel { padding: 5px; }");
    
    testLayout->addLayout(testButtonLayout);
    testLayout->addWidget(progressBar);
    testLayout->addWidget(statusLabel);
    
    mainLayout->addWidget(testGroup);
    
    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    cancelButton = new QPushButton("取消");
    okButton = new QPushButton("确定");
    okButton->setDefault(true);
    
    cancelButton->setStyleSheet(
        "QPushButton { padding: 8px 16px; border: 1px solid #ccc; border-radius: 4px; }"
        "QPushButton:hover { background-color: #f8f9fa; }"
    );
    okButton->setStyleSheet(
        "QPushButton { background-color: #28a745; color: white; font-weight: bold; "
        "padding: 8px 16px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #218838; }"
    );
    
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
    
    mainLayout->addLayout(buttonLayout);
}

void AISummaryConfigDialog::onProviderChanged(const QString& provider) {
    // 更新默认设置
    baseUrlEdit->setText(getDefaultBaseUrl(provider));
    
    // 更新模型选项
    modelCombo->clear();
    QStringList models = getDefaultModels(provider);
    modelCombo->addItems(models);
    
    // 清除状态
    statusLabel->setText("请配置模型参数后测试连接");
    statusLabel->setStyleSheet("QLabel { padding: 5px; }");
}

QString AISummaryConfigDialog::getDefaultBaseUrl(const QString& provider) const {
    if (provider == "OpenAI") {
        return "https://api.openai.com/v1";
    } else if (provider == "硅基流动 (SiliconFlow)") {
        return "https://api.siliconflow.cn/v1";
    } else if (provider == "智谱AI (GLM)") {
        return "https://open.bigmodel.cn/api/paas/v4";
    } else if (provider == "月之暗面 (Kimi)") {
        return "https://api.moonshot.cn/v1";
    }
    return "";
}

QStringList AISummaryConfigDialog::getDefaultModels(const QString& provider) const {
    if (provider == "OpenAI") {
        return {"gpt-4-vision-preview", "gpt-4o", "gpt-4o-mini"};
    } else if (provider == "硅基流动 (SiliconFlow)") {
        return {"OpenGVLab/InternVL2-26B", "OpenGVLab/InternVL2-Llama3-76B", "deepseek-ai/deepseek-vl-7b-chat"};
    } else if (provider == "智谱AI (GLM)") {
        return {"glm-4v-plus", "glm-4v", "glm-4v-flash"};
    } else if (provider == "月之暗面 (Kimi)") {
        return {"moonshot-v1-8k", "moonshot-v1-32k", "moonshot-v1-128k"};
    }
    return {};
}

void AISummaryConfigDialog::onTestConnection() {
    // 获取当前配置
    AISummaryConfig config = getConfig();
    
    if (!config.isValid()) {
        statusLabel->setText("❌ 请填写完整的配置信息");
        statusLabel->setStyleSheet("QLabel { padding: 5px; color: #dc3545; }");
        return;
    }
    
    // 显示进度
    testButton->setEnabled(false);
    progressBar->setVisible(true);
    statusLabel->setText("正在测试连接...");
    statusLabel->setStyleSheet("QLabel { padding: 5px; color: #007bff; }");
    
    // 创建测试请求
    QNetworkRequest request;
    QString url = config.baseUrl;
    if (!url.endsWith("/")) {
        url += "/";
    }
    
    // 根据不同提供商使用不同的测试端点
    if (config.provider == "OpenAI" || config.provider == "硅基流动 (SiliconFlow)" || 
        config.provider == "月之暗面 (Kimi)") {
        url += "models";
    } else if (config.provider == "智谱AI (GLM)") {
        url += "models";
    } else {
        url += "models"; // 默认使用 models 端点
    }
    
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey).toUtf8());
    request.setRawHeader("User-Agent", "AIcp/1.0");
    
    // 发送请求
    if (currentReply) {
        currentReply->abort();
        currentReply->deleteLater();
    }
    
    currentReply = networkManager->get(request);
    connect(currentReply, &QNetworkReply::finished, 
            this, &AISummaryConfigDialog::onNetworkReplyFinished);
    
    // 设置超时
    QTimer::singleShot(10000, this, [this]() {
        if (currentReply && currentReply->isRunning()) {
            currentReply->abort();
        }
    });
}

void AISummaryConfigDialog::onNetworkReplyFinished() {
    testButton->setEnabled(true);
    progressBar->setVisible(false);
    
    if (!currentReply) {
        return;
    }
    
    QNetworkReply::NetworkError error = currentReply->error();
    
    if (error == QNetworkReply::NoError) {
        // 成功
        statusLabel->setText("✅ 连接测试成功！模型配置有效。");
        statusLabel->setStyleSheet("QLabel { padding: 5px; color: #28a745; }");
        
        // 尝试解析响应以验证模型列表
        QByteArray data = currentReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("data")) {
                statusLabel->setText("✅ 连接测试成功！已验证模型列表。");
            }
        }
        
    } else {
        // 失败
        QString errorMsg = currentReply->errorString();
        int httpStatus = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        QString displayMsg;
        if (httpStatus == 401) {
            displayMsg = "❌ API Key无效或权限不足";
        } else if (httpStatus == 403) {
            displayMsg = "❌ 访问被拒绝，请检查API Key权限";
        } else if (httpStatus == 404) {
            displayMsg = "❌ API端点不存在，请检查Base URL";
        } else if (httpStatus >= 500) {
            displayMsg = "❌ 服务器错误，请稍后重试";
        } else {
            displayMsg = QString("❌ 连接失败: %1").arg(errorMsg);
        }
        
        statusLabel->setText(displayMsg);
        statusLabel->setStyleSheet("QLabel { padding: 5px; color: #dc3545; }");
    }
    
    currentReply->deleteLater();
    currentReply = nullptr;
}

AISummaryConfig AISummaryConfigDialog::getConfig() const {
    AISummaryConfig config;
    config.provider = providerCombo->currentText();
    config.baseUrl = baseUrlEdit->text().trimmed();
    config.apiKey = apiKeyEdit->text().trimmed();
    config.modelName = modelCombo->currentText().trimmed();
    config.enabled = true;
    return config;
}

void AISummaryConfigDialog::setConfig(const AISummaryConfig& config) {
    // 设置提供商
    int index = providerCombo->findText(config.provider);
    if (index >= 0) {
        providerCombo->setCurrentIndex(index);
    }
    
    baseUrlEdit->setText(config.baseUrl);
    apiKeyEdit->setText(config.apiKey);
    
    // 设置模型名称
    modelCombo->setCurrentText(config.modelName);
    
    // 更新状态
    if (config.isValid()) {
        statusLabel->setText("配置已加载，建议重新测试连接");
        statusLabel->setStyleSheet("QLabel { padding: 5px; color: #007bff; }");
    }
}
