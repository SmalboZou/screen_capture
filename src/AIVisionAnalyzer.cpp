#include "AIVisionAnalyzer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>
#include <QImageReader>
#include <QDebug>
#include <QMutexLocker>
#include <QThread>

AIVisionAnalyzer::AIVisionAnalyzer(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , currentReply(nullptr)
    , timeoutTimer(new QTimer(this))
    , isAnalyzing(false)
    , currentImageIndex(0)
    , totalImages(0)
{
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, &AIVisionAnalyzer::onNetworkTimeout);
    
    // 设置网络超时
    networkManager->setTransferTimeout(REQUEST_TIMEOUT_MS);
}

AIVisionAnalyzer::~AIVisionAnalyzer() {
    cancelAnalysis();
}

void AIVisionAnalyzer::setConfig(const AISummaryConfig &newConfig) {
    config = newConfig;
}

void AIVisionAnalyzer::analyzeImages(const QStringList &imagePaths) {
    if (isAnalyzing) {
        emit imageAnalysisFinished(false, "已有分析任务在进行中");
        return;
    }
    
    if (!config.isValid()) {
        emit imageAnalysisFinished(false, "AI配置无效，请先配置AI模型");
        return;
    }
    
    if (imagePaths.isEmpty()) {
        emit imageAnalysisFinished(false, "没有图片需要分析");
        return;
    }
    
    // 初始化状态
    imageQueue.clear();
    analysisResults.clear();
    frameDescriptions.clear();
    
    for (const QString &path : imagePaths) {
        if (QFileInfo::exists(path)) {
            imageQueue.enqueue(path);
        }
    }
    
    if (imageQueue.isEmpty()) {
        emit imageAnalysisFinished(false, "没有找到有效的图片文件");
        return;
    }
    
    isAnalyzing = true;
    currentImageIndex = 0;
    totalImages = imageQueue.size();
    
    qDebug() << "开始分析" << totalImages << "张图片";
    
    // 开始处理第一张图片
    processNextImage();
}

void AIVisionAnalyzer::processNextImage() {
    if (!isAnalyzing || imageQueue.isEmpty()) {
        // 所有图片处理完成
        isAnalyzing = false;
        
        if (analysisResults.size() > 0) {
            // 收集成功的描述
            QStringList descriptions;
            for (const auto &result : analysisResults) {
                if (result.success && !result.description.isEmpty()) {
                    descriptions.append(result.description);
                }
            }
            
            if (descriptions.isEmpty()) {
                emit imageAnalysisFinished(false, "没有成功分析的图片");
            } else {
                emit imageAnalysisFinished(true, QString("成功分析了 %1/%2 张图片")
                                         .arg(descriptions.size()).arg(totalImages));
                
                // 生成最终总结
                generateFinalSummary(descriptions);
            }
        } else {
            emit imageAnalysisFinished(false, "图片分析失败");
        }
        return;
    }
    
    QString imagePath = imageQueue.dequeue();
    currentImageIndex++;
    
    emit imageAnalysisProgress(currentImageIndex, totalImages);
    
    qDebug() << "分析图片:" << imagePath << QString("(%1/%2)").arg(currentImageIndex).arg(totalImages);
    
    // 编码图片为base64
    QString base64Image = encodeImageToBase64(imagePath);
    if (base64Image.isEmpty()) {
        FrameAnalysisResult result;
        result.imagePath = imagePath;
        result.success = false;
        result.errorMessage = "无法读取图片文件";
        result.frameIndex = currentImageIndex - 1;
        
        QMutexLocker locker(&resultsMutex);
        analysisResults.append(result);
        
        // 继续处理下一张图片
        QTimer::singleShot(100, this, &AIVisionAnalyzer::processNextImage);
        return;
    }
    
    // 创建API请求
    QJsonObject requestBody;
    if (config.provider == "OpenAI") {
        requestBody = createOpenAIRequest(base64Image);
    } else if (config.provider == "硅基流动 (SiliconFlow)") {
        requestBody = createSiliconFlowRequest(base64Image);
    } else if (config.provider == "智谱AI (GLM)") {
        requestBody = createGLMRequest(base64Image);
    } else if (config.provider == "月之暗面 (Kimi)") {
        requestBody = createKimiRequest(base64Image);
    } else {
        // 默认使用OpenAI格式
        requestBody = createOpenAIRequest(base64Image);
    }
    
    // 发送请求
    QString endpoint = config.baseUrl;
    if (!endpoint.endsWith("/")) {
        endpoint += "/";
    }
    endpoint += "chat/completions";
    
    QNetworkRequest request = createNetworkRequest(endpoint);
    
    if (currentReply) {
        currentReply->abort();
        currentReply->deleteLater();
    }
    
    QJsonDocument doc(requestBody);
    currentReply = networkManager->post(request, doc.toJson());
    
    // 存储当前处理的图片路径
    currentReply->setProperty("imagePath", imagePath);
    currentReply->setProperty("frameIndex", currentImageIndex - 1);
    
    connect(currentReply, &QNetworkReply::finished,
            this, &AIVisionAnalyzer::onImageAnalysisReply);
    
    // 启动超时定时器
    timeoutTimer->start(REQUEST_TIMEOUT_MS);
}

void AIVisionAnalyzer::onImageAnalysisReply() {
    timeoutTimer->stop();
    
    if (!currentReply) {
        return;
    }
    
    QString imagePath = currentReply->property("imagePath").toString();
    int frameIndex = currentReply->property("frameIndex").toInt();
    
    FrameAnalysisResult result;
    result.imagePath = imagePath;
    result.frameIndex = frameIndex;
    
    QNetworkReply::NetworkError error = currentReply->error();
    
    if (error == QNetworkReply::NoError) {
        QByteArray data = currentReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (doc.isObject()) {
            QJsonObject response = doc.object();
            
            QString description;
            if (config.provider == "OpenAI") {
                description = parseOpenAIResponse(response);
            } else if (config.provider == "硅基流动 (SiliconFlow)") {
                description = parseSiliconFlowResponse(response);
            } else if (config.provider == "智谱AI (GLM)") {
                description = parseGLMResponse(response);
            } else if (config.provider == "月之暗面 (Kimi)") {
                description = parseKimiResponse(response);
            } else {
                description = parseOpenAIResponse(response);
            }
            
            if (!description.isEmpty()) {
                result.success = true;
                result.description = description;
                qDebug() << "图片分析成功:" << imagePath << " -> " << description.left(50) + "...";
            } else {
                result.success = false;
                result.errorMessage = "API返回空描述";
                qDebug() << "API返回空描述:" << response;
            }
        } else {
            result.success = false;
            result.errorMessage = "API返回格式错误";
            qDebug() << "API返回格式错误:" << data;
        }
    } else {
        result.success = false;
        result.errorMessage = currentReply->errorString();
        qDebug() << "网络请求失败:" << result.errorMessage;
    }
    
    {
        QMutexLocker locker(&resultsMutex);
        analysisResults.append(result);
    }
    
    currentReply->deleteLater();
    currentReply = nullptr;
    
    // 添加短暂延迟避免API限制
    QTimer::singleShot(RETRY_DELAY_MS, this, &AIVisionAnalyzer::processNextImage);
}

void AIVisionAnalyzer::generateFinalSummary(const QStringList &descriptions) {
    if (descriptions.isEmpty()) {
        emit finalSummaryGenerated(false, "", "没有可用的图片描述");
        return;
    }
    
    frameDescriptions = descriptions;
    
    // 创建总结请求
    QString prompt = QString(
        "请根据以下从视频中提取的连续帧的描述，生成一个完整的视频内容总结。"
        "描述按时间顺序排列，每个描述代表视频中约0.5秒的画面内容。"
        "请总结视频中发生了什么事情，包括主要活动、场景变化和重要细节。"
        "用中文回答，并保持简洁明了。\n\n帧描述列表：\n%1"
    ).arg(descriptions.join("\n"));
    
    QJsonObject requestBody;
    QJsonArray messages;
    
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "你是一个专业的视频内容分析助手，擅长根据视频帧描述生成准确、简洁的视频总结。";
    messages.append(systemMessage);
    
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messages.append(userMessage);
    
    requestBody["model"] = config.modelName;
    requestBody["messages"] = messages;
    requestBody["max_tokens"] = 1000;
    requestBody["temperature"] = 0.3;
    
    // 发送请求
    QString endpoint = config.baseUrl;
    if (!endpoint.endsWith("/")) {
        endpoint += "/";
    }
    endpoint += "chat/completions";
    
    QNetworkRequest request = createNetworkRequest(endpoint);
    
    if (currentReply) {
        currentReply->abort();
        currentReply->deleteLater();
    }
    
    QJsonDocument doc(requestBody);
    currentReply = networkManager->post(request, doc.toJson());
    
    connect(currentReply, &QNetworkReply::finished,
            this, &AIVisionAnalyzer::onSummaryReply);
    
    timeoutTimer->start(REQUEST_TIMEOUT_MS);
}

void AIVisionAnalyzer::onSummaryReply() {
    timeoutTimer->stop();
    
    if (!currentReply) {
        return;
    }
    
    QNetworkReply::NetworkError error = currentReply->error();
    
    if (error == QNetworkReply::NoError) {
        QByteArray data = currentReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (doc.isObject()) {
            QJsonObject response = doc.object();
            
            QString summary;
            if (config.provider == "OpenAI") {
                summary = parseOpenAIResponse(response);
            } else if (config.provider == "硅基流动 (SiliconFlow)") {
                summary = parseSiliconFlowResponse(response);
            } else if (config.provider == "智谱AI (GLM)") {
                summary = parseGLMResponse(response);
            } else if (config.provider == "月之暗面 (Kimi)") {
                summary = parseKimiResponse(response);
            } else {
                summary = parseOpenAIResponse(response);
            }
            
            if (!summary.isEmpty()) {
                emit finalSummaryGenerated(true, summary, "视频内容总结生成成功");
            } else {
                emit finalSummaryGenerated(false, "", "API返回空总结");
            }
        } else {
            emit finalSummaryGenerated(false, "", "API返回格式错误");
        }
    } else {
        emit finalSummaryGenerated(false, "", QString("总结生成失败: %1").arg(currentReply->errorString()));
    }
    
    currentReply->deleteLater();
    currentReply = nullptr;
}

void AIVisionAnalyzer::onNetworkTimeout() {
    if (currentReply) {
        currentReply->abort();
        
        FrameAnalysisResult result;
        result.imagePath = currentReply->property("imagePath").toString();
        result.frameIndex = currentReply->property("frameIndex").toInt();
        result.success = false;
        result.errorMessage = "请求超时";
        
        QMutexLocker locker(&resultsMutex);
        analysisResults.append(result);
        
        currentReply->deleteLater();
        currentReply = nullptr;
    }
    
    // 继续处理下一张图片
    QTimer::singleShot(1000, this, &AIVisionAnalyzer::processNextImage);
}

QString AIVisionAnalyzer::encodeImageToBase64(const QString &imagePath) const {
    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QByteArray imageData = file.readAll();
    return imageData.toBase64();
}

QJsonObject AIVisionAnalyzer::createOpenAIRequest(const QString &base64Image) const {
    QJsonObject requestBody;
    QJsonArray messages;
    
    QJsonObject message;
    message["role"] = "user";
    
    QJsonArray content;
    
    // 文本内容
    QJsonObject textContent;
    textContent["type"] = "text";
    textContent["text"] = "请详细描述这张图片中的内容，包括场景、物体、人物行为和任何重要细节。用中文回答。";
    content.append(textContent);
    
    // 图片内容
    QJsonObject imageContent;
    imageContent["type"] = "image_url";
    
    QJsonObject imageUrl;
    imageUrl["url"] = QString("data:image/jpeg;base64,%1").arg(base64Image);
    imageContent["image_url"] = imageUrl;
    content.append(imageContent);
    
    message["content"] = content;
    messages.append(message);
    
    requestBody["model"] = config.modelName;
    requestBody["messages"] = messages;
    requestBody["max_tokens"] = 500;
    requestBody["temperature"] = 0.3;
    
    return requestBody;
}

QJsonObject AIVisionAnalyzer::createSiliconFlowRequest(const QString &base64Image) const {
    // 硅基流动使用类似OpenAI的格式
    return createOpenAIRequest(base64Image);
}

QJsonObject AIVisionAnalyzer::createGLMRequest(const QString &base64Image) const {
    QJsonObject requestBody;
    QJsonArray messages;
    
    QJsonObject message;
    message["role"] = "user";
    
    QJsonArray content;
    
    // 文本内容
    QJsonObject textContent;
    textContent["type"] = "text";
    textContent["text"] = "请详细描述这张图片中的内容，包括场景、物体、人物行为和任何重要细节。用中文回答。";
    content.append(textContent);
    
    // 图片内容
    QJsonObject imageContent;
    imageContent["type"] = "image_url";
    
    QJsonObject imageUrl;
    imageUrl["url"] = QString("data:image/jpeg;base64,%1").arg(base64Image);
    imageContent["image_url"] = imageUrl;
    content.append(imageContent);
    
    message["content"] = content;
    messages.append(message);
    
    requestBody["model"] = config.modelName;
    requestBody["messages"] = messages;
    requestBody["max_tokens"] = 500;
    requestBody["temperature"] = 0.3;
    
    return requestBody;
}

QJsonObject AIVisionAnalyzer::createKimiRequest(const QString &base64Image) const {
    // Kimi使用类似OpenAI的格式
    return createOpenAIRequest(base64Image);
}

QNetworkRequest AIVisionAnalyzer::createNetworkRequest(const QString &endpoint) const {
    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey).toUtf8());
    request.setRawHeader("User-Agent", "AIcp-VideoSummary/1.0");
    
    return request;
}

QString AIVisionAnalyzer::parseOpenAIResponse(const QJsonObject &response) const {
    if (response.contains("choices") && response["choices"].isArray()) {
        QJsonArray choices = response["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject choice = choices[0].toObject();
            if (choice.contains("message")) {
                QJsonObject message = choice["message"].toObject();
                return message["content"].toString().trimmed();
            }
        }
    }
    return QString();
}

QString AIVisionAnalyzer::parseSiliconFlowResponse(const QJsonObject &response) const {
    // 硅基流动使用相同的响应格式
    return parseOpenAIResponse(response);
}

QString AIVisionAnalyzer::parseGLMResponse(const QJsonObject &response) const {
    // GLM使用相同的响应格式
    return parseOpenAIResponse(response);
}

QString AIVisionAnalyzer::parseKimiResponse(const QJsonObject &response) const {
    // Kimi使用相同的响应格式
    return parseOpenAIResponse(response);
}

void AIVisionAnalyzer::cancelAnalysis() {
    if (currentReply) {
        currentReply->abort();
        currentReply->deleteLater();
        currentReply = nullptr;
    }
    
    timeoutTimer->stop();
    isAnalyzing = false;
    imageQueue.clear();
}

QList<FrameAnalysisResult> AIVisionAnalyzer::getResults() const {
    QMutexLocker locker(&const_cast<QMutex&>(resultsMutex));
    return analysisResults;
}
