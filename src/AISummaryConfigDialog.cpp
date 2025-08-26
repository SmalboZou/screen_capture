#include "AISummaryConfigDialog.h"
#include <QApplication>
#include <QTimer>

AISummaryConfigDialog::AISummaryConfigDialog(QWidget *parent)
    : QDialog(parent)
    , networkManager(new QNetworkAccessManager(this))
    , currentReply(nullptr)
    , modelListReply(nullptr)
{
    setWindowTitle("AI视频内容总结配置");
    setModal(true);
    setMinimumSize(500, 450);
    resize(600, 500);
    
    setupUI();
    
    // 连接信号
    connect(providerCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged),
            this, &AISummaryConfigDialog::onProviderChanged);
    connect(refreshModelsButton, &QPushButton::clicked, this, &AISummaryConfigDialog::onRefreshModelsClicked);
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
    if (modelListReply) {
        modelListReply->abort();
        modelListReply->deleteLater();
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
    QHBoxLayout *modelLayout = new QHBoxLayout();
    modelCombo = new QComboBox();
    modelCombo->setEditable(true);
    modelCombo->setMinimumWidth(200);
    
    refreshModelsButton = new QPushButton("刷新");
    refreshModelsButton->setMaximumWidth(60);
    refreshModelsButton->setStyleSheet(
        "QPushButton { background-color: #6f42c1; color: white; border-radius: 3px; padding: 4px 8px; }"
        "QPushButton:hover { background-color: #5a32a3; }"
        "QPushButton:disabled { background-color: #6c757d; }"
    );
    refreshModelsButton->setToolTip("自动获取可用的视觉模型列表");
    
    modelLayout->addWidget(modelCombo);
    modelLayout->addWidget(refreshModelsButton);
    formLayout->addRow("模型名称:", modelLayout);
    
    // 模型状态标签
    modelStatusLabel = new QLabel("点击'刷新'按钮获取可用模型列表");
    modelStatusLabel->setStyleSheet("QLabel { color: #6c757d; font-size: 11px; }");
    formLayout->addRow("", modelStatusLabel);
    
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
    
    modelStatusLabel->setText("点击'刷新'按钮获取可用模型列表");
    modelStatusLabel->setStyleSheet("QLabel { color: #6c757d; font-size: 11px; }");
    
    // 启用刷新按钮（如果不是自定义提供商）
    refreshModelsButton->setEnabled(provider != "自定义");
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
        return {""};
    } else if (provider == "硅基流动 (SiliconFlow)") {
        return {"deepseek-ai/deepseek-vl2", "Qwen/QVQ-72B-Preview", "Qwen/Qwen2.5-VL-72B-Instruct"};
    } else if (provider == "智谱AI (GLM)") {
        return {""};
    } else if (provider == "月之暗面 (Kimi)") {
        return {""};
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
    QTimer::singleShot(30000, this, [this]() {
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

void AISummaryConfigDialog::onRefreshModelsClicked() {
    QString provider = providerCombo->currentText();
    QString baseUrl = baseUrlEdit->text().trimmed();
    QString apiKey = apiKeyEdit->text().trimmed();
    
    if (baseUrl.isEmpty() || apiKey.isEmpty()) {
        modelStatusLabel->setText("❌ 请先填写Base URL和API Key");
        modelStatusLabel->setStyleSheet("QLabel { color: #dc3545; font-size: 11px; }");
        return;
    }
    
    fetchAvailableModels();
}

void AISummaryConfigDialog::fetchAvailableModels() {
    QString provider = providerCombo->currentText();
    QString baseUrl = baseUrlEdit->text().trimmed();
    QString apiKey = apiKeyEdit->text().trimmed();
    
    if (baseUrl.isEmpty() || apiKey.isEmpty()) {
        return;
    }
    
    // 显示加载状态
    refreshModelsButton->setEnabled(false);
    modelStatusLabel->setText("🔄 正在获取模型列表...");
    modelStatusLabel->setStyleSheet("QLabel { color: #007bff; font-size: 11px; }");
    
    // 创建请求
    QNetworkRequest request;
    QString url = baseUrl;
    if (!url.endsWith("/")) {
        url += "/";
    }
    url += "models";
    
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    request.setRawHeader("User-Agent", "AIcp-VideoSummary/1.0");
    
    // 发送请求
    if (modelListReply) {
        modelListReply->abort();
        modelListReply->deleteLater();
    }
    
    modelListReply = networkManager->get(request);
    connect(modelListReply, &QNetworkReply::finished, 
            this, &AISummaryConfigDialog::onModelListReplyFinished);
    
    // 设置超时
    QTimer::singleShot(30000, this, [this]() {
        if (modelListReply && modelListReply->isRunning()) {
            modelListReply->abort();
        }
    });
}

void AISummaryConfigDialog::onModelListReplyFinished() {
    refreshModelsButton->setEnabled(true);
    
    if (!modelListReply) {
        return;
    }
    
    QNetworkReply::NetworkError error = modelListReply->error();
    
    if (error == QNetworkReply::NoError) {
        QByteArray data = modelListReply->readAll();
        parseModelListResponse(data);
    } else {
        QString errorMsg;
        int httpStatus = modelListReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        if (httpStatus == 401) {
            errorMsg = "❌ API Key无效";
        } else if (httpStatus == 403) {
            errorMsg = "❌ 访问被拒绝";
        } else if (httpStatus == 404) {
            errorMsg = "❌ API端点不存在";
        } else {
            errorMsg = "❌ 获取失败，使用默认模型";
        }
        
        modelStatusLabel->setText(errorMsg);
        modelStatusLabel->setStyleSheet("QLabel { color: #dc3545; font-size: 11px; }");
        
        // 恢复默认模型列表
        QString provider = providerCombo->currentText();
        modelCombo->clear();
        modelCombo->addItems(getDefaultModels(provider));
    }
    
    modelListReply->deleteLater();
    modelListReply = nullptr;
}

void AISummaryConfigDialog::parseModelListResponse(const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        modelStatusLabel->setText("❌ 响应格式错误");
        modelStatusLabel->setStyleSheet("QLabel { color: #dc3545; font-size: 11px; }");
        return;
    }
    
    QJsonObject response = doc.object();
    if (!response.contains("data") || !response["data"].isArray()) {
        modelStatusLabel->setText("❌ 无法解析模型列表");
        modelStatusLabel->setStyleSheet("QLabel { color: #dc3545; font-size: 11px; }");
        return;
    }
    
    QJsonArray models = response["data"].toArray();
    QStringList visionModels;
    QStringList allModels;
    
    // 保存当前选择的模型
    QString currentModel = modelCombo->currentText();
    
    for (const QJsonValue& value : models) {
        if (value.isObject()) {
            QJsonObject model = value.toObject();
            QString modelId = model["id"].toString();
            
            if (!modelId.isEmpty()) {
                allModels.append(modelId);
                
                // 检查是否为视觉模型
                if (isVisionModel(modelId)) {
                    visionModels.append(modelId);
                }
            }
        }
    }
    
    // 更新模型下拉框
    modelCombo->clear();
    
    if (!visionModels.isEmpty()) {
        modelCombo->addItems(visionModels);
        modelStatusLabel->setText(QString("✅ 已获取 %1 个视觉模型").arg(visionModels.size()));
        modelStatusLabel->setStyleSheet("QLabel { color: #28a745; font-size: 11px; }");
    } else {
        // 如果没有找到视觉模型，显示所有模型但给出提示
        modelCombo->addItems(allModels);
        modelStatusLabel->setText(QString("⚠️ 已获取 %1 个模型（请确认支持视觉功能）").arg(allModels.size()));
        modelStatusLabel->setStyleSheet("QLabel { color: #ffc107; font-size: 11px; }");
    }
    
    // 尝试恢复之前选择的模型
    if (!currentModel.isEmpty()) {
        int index = modelCombo->findText(currentModel);
        if (index >= 0) {
            modelCombo->setCurrentIndex(index);
        }
    }
}

bool AISummaryConfigDialog::isVisionModel(const QString& modelName) const {
    QString lower = modelName.toLower();
    
    // OpenAI视觉模型
    if (lower.contains("gpt-4") && (lower.contains("vision") || lower.contains("4o"))) {
        return true;
    }
    
    // 硅基流动视觉模型
    if (lower.contains("internvl") || lower.contains("deepseek-vl") || 
        lower.contains("cogvlm") || lower.contains("qvq") ||
        lower.contains("vl") || lower.contains("stepfun-ai/step3")) {
        return true;
    }
    
    // 智谱AI视觉模型
    if (lower.contains("glm") && lower.contains("v")) {
        return true;
    }
    
    // 月之暗面（Kimi）暂时没有专门的视觉模型标识，大部分都支持
    QString provider = providerCombo->currentText();
    if (provider == "月之暗面 (Kimi)" && lower.contains("moonshot")) {
        return true;
    }
    
    // 其他常见视觉模型关键词
    if (lower.contains("vision") || lower.contains("visual") || 
        lower.contains("multimodal") || lower.contains("llava") ||
        lower.contains("blip") || lower.contains("flamingo")) {
        return true;
    }
    
    return false;
}
