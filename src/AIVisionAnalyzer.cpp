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
#include <QRegularExpression>

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
        
        // 添加调试信息，帮助诊断thinking模型的响应
        qDebug() << "API响应数据大小:" << data.size() << "字节";
        
        if (data.isEmpty()) {
            result.success = false;
            result.errorMessage = "API返回空响应";
            qDebug() << "API返回空响应";
        } else {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
            
            if (parseError.error != QJsonParseError::NoError) {
                result.success = false;
                result.errorMessage = QString("JSON解析错误: %1").arg(parseError.errorString());
                qDebug() << "JSON解析失败:" << parseError.errorString();
                qDebug() << "响应数据前1000字符:" << data.left(1000);
            } else if (doc.isObject()) {
                QJsonObject response = doc.object();
                
                // 检查是否有错误信息
                if (response.contains("error")) {
                    QJsonObject errorObj = response["error"].toObject();
                    result.success = false;
                    result.errorMessage = QString("API错误: %1").arg(errorObj["message"].toString());
                    qDebug() << "API返回错误:" << errorObj;
                } else {
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
                        qDebug() << "API返回空描述，完整响应:" << response;
                    }
                }
            } else {
                result.success = false;
                result.errorMessage = "API返回格式错误";
                qDebug() << "API返回格式错误，响应数据:" << data;
            }
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
        
        qDebug() << "总结生成API响应数据大小:" << data.size() << "字节";
        
        if (data.isEmpty()) {
            emit finalSummaryGenerated(false, "", "API返回空响应");
        } else {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
            
            if (parseError.error != QJsonParseError::NoError) {
                emit finalSummaryGenerated(false, "", QString("JSON解析错误: %1").arg(parseError.errorString()));
                qDebug() << "总结生成JSON解析失败:" << parseError.errorString();
            } else if (doc.isObject()) {
                QJsonObject response = doc.object();
                
                // 检查是否有错误信息
                if (response.contains("error")) {
                    QJsonObject errorObj = response["error"].toObject();
                    emit finalSummaryGenerated(false, "", QString("API错误: %1").arg(errorObj["message"].toString()));
                    qDebug() << "总结生成API返回错误:" << errorObj;
                } else {
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
                        qDebug() << "总结生成API返回空总结，完整响应:" << response;
                    }
                }
            } else {
                emit finalSummaryGenerated(false, "", "API返回格式错误");
                qDebug() << "总结生成API返回格式错误，响应数据:" << data;
            }
        }
    } else {
        emit finalSummaryGenerated(false, "", QString("总结生成失败: %1").arg(currentReply->errorString()));
    }
    
    currentReply->deleteLater();
    currentReply = nullptr;
}

void AIVisionAnalyzer::onNetworkTimeout() {
    qDebug() << "网络请求超时，正在处理...";
    
    if (currentReply) {
        QString imagePath = currentReply->property("imagePath").toString();
        int frameIndex = currentReply->property("frameIndex").toInt();
        
        qDebug() << "超时的请求 - 图片:" << imagePath << "帧索引:" << frameIndex;
        
        // 安全地中止请求
        currentReply->abort();
        
        FrameAnalysisResult result;
        result.imagePath = imagePath;
        result.frameIndex = frameIndex;
        result.success = false;
        result.errorMessage = "请求超时 - thinking模型可能需要更长时间，请考虑增加超时设置";
        
        {
            QMutexLocker locker(&resultsMutex);
            analysisResults.append(result);
        }
        
        currentReply->deleteLater();
        currentReply = nullptr;
    }
    
    // 继续处理下一张图片，避免整个流程卡住
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
    
    // 文本内容 - 针对thinking模型优化
    QJsonObject textContent;
    textContent["type"] = "text";
    
    // 检查是否是thinking模型，给出更直接的指令
    QString prompt;
    if (config.modelName.toLower().contains("thinking") || config.modelName.toLower().contains("o1")) {
        prompt = "请直接描述这张图片中的内容，包括场景、物体、人物行为和重要细节。请用中文简洁回答，不需要思考过程。";
    } else {
        prompt = "请详细描述这张图片中的内容，包括场景、物体、人物行为和任何重要细节。用中文回答。";
    }
    
    textContent["text"] = prompt;
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
                QString content = message["content"].toString().trimmed();
                
                // 对于thinking模型，可能包含思考过程和最终答案
                // 尝试提取最终的描述内容
                if (content.contains("<answer>") && content.contains("</answer>")) {
                    // 提取<answer>标签内的内容
                    int startPos = content.indexOf("<answer>") + 8;
                    int endPos = content.indexOf("</answer>");
                    if (endPos > startPos) {
                        content = content.mid(startPos, endPos - startPos).trimmed();
                    }
                } else if (content.contains("答案:") || content.contains("答案：")) {
                    // 寻找"答案:"后的内容
                    QStringList lines = content.split('\n');
                    for (int i = 0; i < lines.size(); i++) {
                        if (lines[i].contains("答案:") || lines[i].contains("答案：")) {
                            QStringList remainingLines = lines.mid(i);
                            QString remaining = remainingLines.join('\n');
                            QRegularExpression re("^.*答案[：:]\\s*");
                            content = remaining.replace(re, "").trimmed();
                            break;
                        }
                    }
                }
                
                return content;
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
