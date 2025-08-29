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
    
    // 分析完成后删除临时图片文件
    QFile tempImageFile(imagePath);
    if (tempImageFile.exists()) {
        if (tempImageFile.remove()) {
            qDebug() << QString("已删除临时图片文件: %1").arg(QFileInfo(imagePath).fileName());
        } else {
            qWarning() << QString("删除临时图片文件失败: %1").arg(imagePath);
        }
    }
    
    currentReply->deleteLater();
    currentReply = nullptr;
    
    // 添加短暂延迟避免API限制
    QTimer::singleShot(RETRY_DELAY_MS, this, &AIVisionAnalyzer::processNextImage);
}

void AIVisionAnalyzer::generateFinalSummary(const QStringList &descriptions) {
    if (descriptions.isEmpty()) {
        qWarning() << "没有可用的图片描述用于生成总结";
        emit finalSummaryGenerated(false, "", "没有可用的图片描述");
        return;
    }
    
    qDebug() << QString("开始生成最终总结，共有 %1 个描述").arg(descriptions.size());
    qDebug() << "使用的AI配置 - 提供商:" << config.provider << "视觉模型:" << config.visionModelName << "总结模型:" << config.summaryModelName;
    
    frameDescriptions = descriptions;
    
    // 判断是否需要分批处理
    const int BATCH_SIZE = 30; // 每30帧为一批
    if (descriptions.size() <= BATCH_SIZE) {
        // 如果描述数量少于等于30个，直接生成总结
        qDebug() << "描述数量较少，直接生成总结";
        generateDirectSummary(descriptions);
    } else {
        // 分批处理
        qDebug() << QString("描述数量较多（%1个），开始分批处理，每批%2个").arg(descriptions.size()).arg(BATCH_SIZE);
        
        // 初始化批处理状态
        batchSummaries.clear();
        currentBatchIndex = 0;
        totalBatches = (descriptions.size() + BATCH_SIZE - 1) / BATCH_SIZE; // 向上取整
        
        qDebug() << QString("总共需要处理 %1 批").arg(totalBatches);
        
        // 开始处理第一批
        processBatch(0, BATCH_SIZE);
    }
}

void AIVisionAnalyzer::processBatch(int batchIndex, int batchSize) {
    int startIndex = batchIndex * batchSize;
    int endIndex = qMin(startIndex + batchSize, frameDescriptions.size());
    
    if (startIndex >= frameDescriptions.size()) {
        // 所有批次处理完成，生成最终总结
        qDebug() << QString("所有批次处理完成，共%1批，开始生成最终合并总结").arg(batchSummaries.size());
        generateFinalMergedSummary();
        return;
    }
    
    QStringList batchDescriptions = frameDescriptions.mid(startIndex, endIndex - startIndex);
    
    qDebug() << QString("处理第%1批（共%2批），包含第%3到第%4帧的描述")
                .arg(batchIndex + 1).arg(totalBatches).arg(startIndex + 1).arg(endIndex);
    
    // 创建批次总结请求
    QString prompt = QString(
        "你是一个专业的视频内容分析助手。请根据以下按时间顺序的屏幕截图描述，生成这一段视频内容的简洁总结。\n\n"
        "背景信息：\n"
        "- 这是视频的第%1段内容（共%2段）\n"
        "- 描述按时间顺序排列，前面的描述和后面的描述存在时序关系\n"
        "- 请专注于总结这一段的主要活动、操作流程和关键信息\n\n"
        "输出要求：\n"
        "1. 用中文回答\n"
        "2. 总结应该简洁明了，150-200字\n"
        "3. 突出这一段的主要活动和操作步骤\n"
        "4. 直接给出总结内容，不需要额外的解释\n\n"
        "这一段的屏幕内容描述：\n%3"
    ).arg(batchIndex + 1).arg(totalBatches).arg(batchDescriptions.join("\n\n"));
    
    QJsonObject requestBody;
    QJsonArray messages;
    
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "你是一个专业的视频内容分析和总结专家。你的任务是根据用户提供的屏幕截图序列描述，生成准确、简洁的视频段落总结。";
    messages.append(systemMessage);
    
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messages.append(userMessage);
    
    requestBody["model"] = config.summaryModelName.isEmpty() ? config.visionModelName : config.summaryModelName;
    requestBody["messages"] = messages;
    requestBody["max_tokens"] = 500; // 批次总结使用较小的token数
    requestBody["temperature"] = 0.3;
    
    // 对于某些模型，添加特殊参数
    if (config.provider.contains("智谱") || config.provider.contains("GLM")) {
        requestBody["stream"] = false;
    } else if (config.provider.contains("Kimi") || config.provider.contains("月之暗面")) {
        requestBody["use_search"] = false;
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
    
    // 设置当前处理的批次索引
    currentReply->setProperty("batchIndex", batchIndex);
    currentReply->setProperty("requestType", "batchSummary");
    
    connect(currentReply, &QNetworkReply::finished,
            this, &AIVisionAnalyzer::onBatchSummaryReply);
    
    timeoutTimer->start(REQUEST_TIMEOUT_MS);
}

void AIVisionAnalyzer::onBatchSummaryReply() {
    timeoutTimer->stop();
    
    if (!currentReply) {
        qWarning() << "批次总结回复为空";
        return;
    }
    
    int batchIndex = currentReply->property("batchIndex").toInt();
    
    QNetworkReply::NetworkError error = currentReply->error();
    qDebug() << QString("第%1批总结API响应").arg(batchIndex + 1) << "- 网络错误代码:" << error;
    
    if (error == QNetworkReply::NoError) {
        QByteArray data = currentReply->readAll();
        
        if (!data.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
            
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject response = doc.object();
                
                if (response.contains("error")) {
                    QJsonObject errorObj = response["error"].toObject();
                    qWarning() << QString("第%1批总结API返回错误:").arg(batchIndex + 1) << errorObj;
                    // 继续处理下一批
                } else {
                    QString batchSummary;
                    if (config.provider == "OpenAI") {
                        batchSummary = parseOpenAIResponse(response);
                    } else if (config.provider == "硅基流动 (SiliconFlow)") {
                        batchSummary = parseSiliconFlowResponse(response);
                    } else if (config.provider == "智谱AI (GLM)") {
                        batchSummary = parseGLMResponse(response);
                    } else if (config.provider == "月之暗面 (Kimi)") {
                        batchSummary = parseKimiResponse(response);
                    } else {
                        batchSummary = parseOpenAIResponse(response);
                    }
                    
                    if (!batchSummary.isEmpty()) {
                        qDebug() << QString("第%1批总结生成成功，长度:%2字符").arg(batchIndex + 1).arg(batchSummary.length());
                        
                        // 存储批次总结
                        if (batchSummaries.size() <= batchIndex) {
                            batchSummaries.resize(batchIndex + 1);
                        }
                        batchSummaries[batchIndex] = batchSummary;
                    } else {
                        qWarning() << QString("第%1批总结解析后为空").arg(batchIndex + 1);
                    }
                }
            } else {
                qWarning() << QString("第%1批总结JSON解析失败:").arg(batchIndex + 1) << parseError.errorString();
            }
        } else {
            qWarning() << QString("第%1批总结API返回空响应").arg(batchIndex + 1);
        }
    } else {
        qWarning() << QString("第%1批总结网络请求失败:").arg(batchIndex + 1) << currentReply->errorString();
    }
    
    currentReply->deleteLater();
    currentReply = nullptr;
    
    // 处理下一批
    currentBatchIndex++;
    
    // 添加延迟避免API限制
    QTimer::singleShot(RETRY_DELAY_MS, this, [this]() {
        processBatch(currentBatchIndex, 30);
    });
}

void AIVisionAnalyzer::generateFinalMergedSummary() {
    // 过滤掉空的批次总结
    QStringList validBatchSummaries;
    for (const QString &summary : batchSummaries) {
        if (!summary.trimmed().isEmpty()) {
            validBatchSummaries.append(summary.trimmed());
        }
    }
    
    if (validBatchSummaries.isEmpty()) {
        qWarning() << "没有有效的批次总结";
        emit finalSummaryGenerated(false, "", "批次总结生成失败");
        return;
    }
    
    qDebug() << QString("开始生成最终合并总结，基于%1个批次总结").arg(validBatchSummaries.size());
    
    // 创建最终合并总结请求
    QString prompt = QString(
        "你是一个专业的视频内容分析助手。请根据以下按时间顺序的视频段落总结，生成一个完整的视频内容总结。\n\n"
        "背景信息：\n"
        "- 这些是用户合法录制的屏幕视频的分段总结\n"
        "- 段落按时间顺序排列，前面的段落先发生，后面的段落后发生\n"
        "- 请将这些段落整合成一个连贯的完整视频总结\n\n"
        "输出要求：\n"
        "1. 用中文回答\n"
        "2. 总结应该完整而简洁，300-500字\n"
        "3. 体现整个视频的主要流程和关键活动\n"
        "4. 保持逻辑连贯性，突出时间顺序\n"
        "5. 直接给出总结内容，不需要额外的解释\n\n"
        "视频段落总结：\n%1"
    ).arg(validBatchSummaries.join("\n\n"));
    
    QJsonObject requestBody;
    QJsonArray messages;
    
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "你是一个专业的视频内容分析和总结专家。你的任务是将多个视频段落总结整合成一个完整、连贯的视频总结。";
    messages.append(systemMessage);
    
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messages.append(userMessage);
    
    requestBody["model"] = config.summaryModelName.isEmpty() ? config.visionModelName : config.summaryModelName;
    requestBody["messages"] = messages;
    requestBody["max_tokens"] = 1000; // 最终总结使用更多token
    requestBody["temperature"] = 0.3;
    
    // 对于某些模型，添加特殊参数
    if (config.provider.contains("智谱") || config.provider.contains("GLM")) {
        requestBody["stream"] = false;
    } else if (config.provider.contains("Kimi") || config.provider.contains("月之暗面")) {
        requestBody["use_search"] = false;
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
    
    currentReply->setProperty("requestType", "finalSummary");
    
    connect(currentReply, &QNetworkReply::finished,
            this, &AIVisionAnalyzer::onSummaryReply);
    
    timeoutTimer->start(REQUEST_TIMEOUT_MS);
}

void AIVisionAnalyzer::generateDirectSummary(const QStringList &descriptions) {
    // 直接生成总结的方法（原有逻辑）
    QString prompt = QString(
        "你是一个专业的视频内容分析助手。请根据以下按时间顺序的屏幕截图描述，生成一个简洁明了的视频内容总结。\n\n"
        "背景信息：\n"
        "- 这是用户合法录制的屏幕视频内容\n"
        "- 描述按时间顺序排列，前面的描述和后面的描述存在一个时序关系，前面的描述的事件先发生，后面的描述后发生。\n"
        "- 请专注于总结主要活动、操作流程和关键信息\n\n"
        "输出要求：\n"
        "1. 用中文回答\n"
        "2. 总结应该简洁明了，100-150字\n"
        "3. 突出主要活动和操作步骤\n"
        "4. 直接给出总结内容，不需要额外的解释\n\n"
        "屏幕内容描述：\n%1"
    ).arg(descriptions.join("\n\n"));
    
    qDebug() << QString("直接总结提示词长度: %1 字符").arg(prompt.length());
    
    QJsonObject requestBody;
    QJsonArray messages;
    
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "你是一个专业的视频内容分析和总结专家。你的任务是根据用户提供的屏幕截图序列描述，生成准确、简洁的视频内容总结。请专注于内容的逻辑性和可读性。描述按时间顺序排列，前面的描述和后面的描述存在一个时序关系，前面的描述的事件先发生，后面的描述后发生。";
    messages.append(systemMessage);
    
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messages.append(userMessage);
    
    requestBody["model"] = config.summaryModelName.isEmpty() ? config.visionModelName : config.summaryModelName;
    requestBody["messages"] = messages;
    requestBody["max_tokens"] = 800; // 直接总结使用适中的token数
    requestBody["temperature"] = 0.3;
    
    // 对于某些模型，添加特殊参数
    if (config.provider.contains("智谱") || config.provider.contains("GLM")) {
        requestBody["stream"] = false;
    } else if (config.provider.contains("Kimi") || config.provider.contains("月之暗面")) {
        requestBody["use_search"] = false;
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
    
    currentReply->setProperty("requestType", "directSummary");
    
    connect(currentReply, &QNetworkReply::finished,
            this, &AIVisionAnalyzer::onSummaryReply);
    
    timeoutTimer->start(REQUEST_TIMEOUT_MS);
}

void AIVisionAnalyzer::onSummaryReply() {
    timeoutTimer->stop();
    
    if (!currentReply) {
        qWarning() << "总结回复为空";
        return;
    }
    
    QString requestType = currentReply->property("requestType").toString();
    
    QNetworkReply::NetworkError error = currentReply->error();
    qDebug() << "总结API响应 (" << requestType << ") - 网络错误代码:" << error;
    qDebug() << "总结API响应 (" << requestType << ") - HTTP状态码:" << currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    if (error == QNetworkReply::NoError) {
        QByteArray data = currentReply->readAll();
        
        qDebug() << "总结生成API响应数据大小:" << data.size() << "字节";
        qDebug() << "总结生成API响应头:" << currentReply->rawHeaderPairs();
        
        if (data.isEmpty()) {
            qWarning() << "总结API返回空响应";
            emit finalSummaryGenerated(false, "", "API返回空响应");
        } else {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
            
            if (parseError.error != QJsonParseError::NoError) {
                qWarning() << "总结生成JSON解析失败:" << parseError.errorString();
                qDebug() << "响应数据前1000字符:" << data.left(1000);
                emit finalSummaryGenerated(false, "", QString("JSON解析错误: %1").arg(parseError.errorString()));
            } else if (doc.isObject()) {
                QJsonObject response = doc.object();
                qDebug() << "总结生成API成功返回JSON对象，键值:" << response.keys();
                
                // 检查是否有错误信息
                if (response.contains("error")) {
                    QJsonObject errorObj = response["error"].toObject();
                    QString errorMsg = QString("API错误: %1").arg(errorObj["message"].toString());
                    qWarning() << "总结生成API返回错误:" << errorObj;
                    emit finalSummaryGenerated(false, "", errorMsg);
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
                    
                    qDebug() << "解析后的总结长度:" << summary.length() << "字符";
                    qDebug() << "解析后的总结前200字符:" << summary.left(200) + "...";
                    
                    if (!summary.isEmpty()) {
                        QString successMessage;
                        if (requestType == "directSummary") {
                            successMessage = "视频内容总结生成成功";
                        } else if (requestType == "finalSummary") {
                            successMessage = "分批视频内容总结合并完成";
                        } else {
                            successMessage = "视频内容总结生成成功";
                        }
                        
                        qDebug() << "总结生成成功 (" << requestType << ")";
                        emit finalSummaryGenerated(true, summary, successMessage);
                    } else {
                        qWarning() << "总结解析后为空";
                        qDebug() << "完整响应对象:" << response;
                        emit finalSummaryGenerated(false, "", "API返回空总结");
                    }
                }
            } else {
                qWarning() << "总结生成API返回格式错误";
                qDebug() << "响应数据:" << data;
                emit finalSummaryGenerated(false, "", "API返回格式错误");
            }
        }
    } else {
        QString errorMsg = QString("总结生成失败: %1").arg(currentReply->errorString());
        qWarning() << errorMsg;
        qDebug() << "网络错误详情 - 状态码:" << currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "网络错误详情 - 响应数据:" << currentReply->readAll();
        emit finalSummaryGenerated(false, "", errorMsg);
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
        
        // 超时时也删除临时图片文件
        QFile tempImageFile(imagePath);
        if (tempImageFile.exists()) {
            if (tempImageFile.remove()) {
                qDebug() << QString("已删除超时的临时图片文件: %1").arg(QFileInfo(imagePath).fileName());
            } else {
                qWarning() << QString("删除超时的临时图片文件失败: %1").arg(imagePath);
            }
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
    if (config.visionModelName.toLower().contains("thinking") || config.visionModelName.toLower().contains("o1")) {
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
    requestBody["max_tokens"] = 200;
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
    
    requestBody["model"] = config.visionModelName;
    requestBody["messages"] = messages;
    requestBody["max_tokens"] = 200;
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
    qDebug() << "解析OpenAI响应，包含的键:" << response.keys();
    
    if (response.contains("choices") && response["choices"].isArray()) {
        QJsonArray choices = response["choices"].toArray();
        qDebug() << "choices数组长度:" << choices.size();
        
        if (!choices.isEmpty()) {
            QJsonObject choice = choices[0].toObject();
            qDebug() << "第一个choice包含的键:" << choice.keys();
            
            if (choice.contains("message")) {
                QJsonObject message = choice["message"].toObject();
                qDebug() << "message包含的键:" << message.keys();
                
                QString content = message["content"].toString().trimmed();
                qDebug() << "原始content长度:" << content.length();
                qDebug() << "原始content前500字符:" << content.left(500) + "...";
                
                if (content.isEmpty()) {
                    qWarning() << "message.content为空";
                    return QString();
                }
                
                // 对于thinking模型，可能包含思考过程和最终答案
                // 尝试提取最终的描述内容
                if (content.contains("<answer>") && content.contains("</answer>")) {
                    // 提取<answer>标签内的内容
                    int startPos = content.indexOf("<answer>") + 8;
                    int endPos = content.indexOf("</answer>");
                    if (endPos > startPos) {
                        QString extractedContent = content.mid(startPos, endPos - startPos).trimmed();
                        qDebug() << "从<answer>标签中提取的内容长度:" << extractedContent.length();
                        content = extractedContent;
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
                            qDebug() << "从答案标识符后提取的内容长度:" << content.length();
                            break;
                        }
                    }
                } else if (content.contains("## ") || content.contains("### ")) {
                    // 如果包含Markdown标题，说明是正常的总结格式，直接使用
                    qDebug() << "检测到Markdown格式的总结内容";
                } else if (content.length() < 50) {
                    // 如果内容太短，可能是错误或不完整的响应
                    qWarning() << "响应内容过短，可能存在问题:" << content;
                }
                
                qDebug() << "最终解析的content长度:" << content.length();
                return content;
            } else {
                qWarning() << "choice中不包含message字段";
            }
        } else {
            qWarning() << "choices数组为空";
        }
    } else {
        qWarning() << "响应中不包含choices字段或choices不是数组";
    }
    
    qDebug() << "parseOpenAIResponse返回空字符串";
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
        // 获取当前正在处理的图片路径
        QString currentImagePath = currentReply->property("imagePath").toString();
        
        currentReply->abort();
        currentReply->deleteLater();
        currentReply = nullptr;
        
        // 删除当前正在处理的图片文件
        if (!currentImagePath.isEmpty()) {
            QFile tempImageFile(currentImagePath);
            if (tempImageFile.exists()) {
                if (tempImageFile.remove()) {
                    qDebug() << QString("已删除取消分析的临时图片文件: %1").arg(QFileInfo(currentImagePath).fileName());
                } else {
                    qWarning() << QString("删除取消分析的临时图片文件失败: %1").arg(currentImagePath);
                }
            }
        }
    }
    
    timeoutTimer->stop();
    isAnalyzing = false;
    
    // 清理队列中剩余的图片文件
    while (!imageQueue.isEmpty()) {
        QString imagePath = imageQueue.dequeue();
        QFile tempImageFile(imagePath);
        if (tempImageFile.exists()) {
            if (tempImageFile.remove()) {
                qDebug() << QString("已删除队列中的临时图片文件: %1").arg(QFileInfo(imagePath).fileName());
            } else {
                qWarning() << QString("删除队列中的临时图片文件失败: %1").arg(imagePath);
            }
        }
    }
    
    imageQueue.clear();
}

QList<FrameAnalysisResult> AIVisionAnalyzer::getResults() const {
    QMutexLocker locker(&const_cast<QMutex&>(resultsMutex));
    return analysisResults;
}
