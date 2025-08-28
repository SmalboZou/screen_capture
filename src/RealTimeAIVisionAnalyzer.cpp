#include "RealTimeAIVisionAnalyzer.h"
#include "AIVisionAnalyzer.h"
#include <QDebug>
#include <QMutexLocker>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QCoreApplication>

RealTimeAIVisionAnalyzer::RealTimeAIVisionAnalyzer(QObject *parent)
    : QObject(parent)
    , processTimer(new QTimer(this))
    , realTimeAnalyzing(false)
    , processingQueue(false)
    , summaryAnalyzer(nullptr)
{
    // 连接处理定时器
    connect(processTimer, &QTimer::timeout, this, &RealTimeAIVisionAnalyzer::processNextFrame);
    
    // 设置处理间隔（避免过于频繁的API调用）
    processTimer->setInterval(100); // 100ms检查一次队列
}

RealTimeAIVisionAnalyzer::~RealTimeAIVisionAnalyzer() {
    cancelAnalysis();
    if (summaryAnalyzer) {
        summaryAnalyzer->deleteLater();
        summaryAnalyzer = nullptr;
    }
}

void RealTimeAIVisionAnalyzer::setConfig(const AISummaryConfig &newConfig) {
    config = newConfig;
}

void RealTimeAIVisionAnalyzer::addFrameForAnalysis(const QString &framePath, double timestamp) {
    if (!realTimeAnalyzing || !isConfigValid()) {
        return;
    }
    
    QMutexLocker locker(&queueMutex);
    
    FrameAnalysisTask task;
    task.framePath = framePath;
    task.timestamp = timestamp;
    task.processed = false;
    
    frameQueue.enqueue(task);
    
    qDebug() << QString("添加帧到分析队列: %1 (时间戳: %2s, 队列长度: %3)")
                .arg(QFileInfo(framePath).fileName())
                .arg(timestamp, 0, 'f', 1)
                .arg(frameQueue.size());
}

void RealTimeAIVisionAnalyzer::startRealTimeAnalysis() {
    if (realTimeAnalyzing) {
        qWarning() << "实时分析已在进行中";
        return;
    }
    
    if (!isConfigValid()) {
        qWarning() << "AI配置无效，无法开始实时分析";
        return;
    }
    
    qDebug() << "开始实时AI视觉分析";
    
    realTimeAnalyzing = true;
    processingQueue = true;
    
    // 清空之前的数据
    {
        QMutexLocker locker(&queueMutex);
        frameQueue.clear();
        completedAnalyses.clear();
    }
    
    // 启动处理定时器
    processTimer->start();
}

void RealTimeAIVisionAnalyzer::stopAndGenerateFinalSummary() {
    if (!realTimeAnalyzing) {
        return;
    }
    
    qDebug() << "停止实时分析并生成最终总结";
    
    realTimeAnalyzing = false;
    processTimer->stop();
    
    // 处理剩余的队列项目
    while (!frameQueue.isEmpty()) {
        processNextFrame();
    }
    
    processingQueue = false;
    
    // 生成最终总结
    generateFinalSummary();
}

bool RealTimeAIVisionAnalyzer::isRealTimeAnalyzing() const {
    return realTimeAnalyzing;
}

void RealTimeAIVisionAnalyzer::cancelAnalysis() {
    if (realTimeAnalyzing) {
        qDebug() << "取消实时AI分析";
        
        realTimeAnalyzing = false;
        processingQueue = false;
        processTimer->stop();
        
        QMutexLocker locker(&queueMutex);
        
        // 清理队列中剩余的图片文件
        while (!frameQueue.isEmpty()) {
            FrameAnalysisTask task = frameQueue.dequeue();
            QFile tempImageFile(task.framePath);
            if (tempImageFile.exists()) {
                if (tempImageFile.remove()) {
                    qDebug() << QString("已删除队列中的临时图片文件: %1").arg(QFileInfo(task.framePath).fileName());
                } else {
                    qWarning() << QString("删除队列中的临时图片文件失败: %1").arg(task.framePath);
                }
            }
        }
        
        frameQueue.clear();
        completedAnalyses.clear();
    }
    
    // 取消总结生成
    if (summaryAnalyzer) {
        summaryAnalyzer->cancelAnalysis();
        summaryAnalyzer->deleteLater();
        summaryAnalyzer = nullptr;
    }
}

void RealTimeAIVisionAnalyzer::processNextFrame() {
    if (!processingQueue) {
        return;
    }
    
    FrameAnalysisTask task;
    bool hasTask = false;
    
    // 从队列中取出任务
    {
        QMutexLocker locker(&queueMutex);
        if (!frameQueue.isEmpty()) {
            task = frameQueue.dequeue();
            hasTask = true;
        }
    }
    
    if (!hasTask) {
        return; // 队列为空
    }
    
    // 检查文件是否存在
    if (!QFileInfo::exists(task.framePath)) {
        qWarning() << "帧文件不存在:" << task.framePath;
        return;
    }
    
    qDebug() << QString("开始分析帧: %1 (时间戳: %2s)")
                .arg(QFileInfo(task.framePath).fileName())
                .arg(task.timestamp, 0, 'f', 1);
    
    // 进行AI分析
    QString analysis = analyzeImageWithAI(task.framePath);
    
    if (!analysis.isEmpty()) {
        task.analysis = analysis;
        task.processed = true;
        
        // 保存到已完成列表
        completedAnalyses.append(task);
        
        qDebug() << QString("帧分析完成: %1 -> %2")
                    .arg(QFileInfo(task.framePath).fileName())
                    .arg(analysis.left(50) + (analysis.length() > 50 ? "..." : ""));
        
        // 发出实时分析信号
        emit realTimeFrameAnalyzed(task.framePath, analysis, task.timestamp);
        
        // 分析完成后删除临时图片文件
        QFile tempImageFile(task.framePath);
        if (tempImageFile.exists()) {
            if (tempImageFile.remove()) {
                qDebug() << QString("已删除临时图片文件: %1").arg(QFileInfo(task.framePath).fileName());
            } else {
                qWarning() << QString("删除临时图片文件失败: %1").arg(task.framePath);
            }
        }
    } else {
        qWarning() << "帧分析失败:" << task.framePath;
        
        // 即使分析失败，也尝试删除临时文件
        QFile tempImageFile(task.framePath);
        if (tempImageFile.exists()) {
            if (tempImageFile.remove()) {
                qDebug() << QString("已删除分析失败的临时图片文件: %1").arg(QFileInfo(task.framePath).fileName());
            } else {
                qWarning() << QString("删除分析失败的临时图片文件失败: %1").arg(task.framePath);
            }
        }
    }
}

QString RealTimeAIVisionAnalyzer::analyzeImageWithAI(const QString &imagePath) {
    if (!isConfigValid()) {
        return QString();
    }
    
    // 创建临时的AIVisionAnalyzer进行单张图片分析
    AIVisionAnalyzer analyzer;
    analyzer.setConfig(config);
    
    // 使用同步方式分析单张图片
    QStringList singleImageList;
    singleImageList << imagePath;
    
    QString result;
    
    // 创建一个临时的事件循环来等待分析完成
    QObject tempContext;
    bool analysisCompleted = false;
    
    QObject::connect(&analyzer, &AIVisionAnalyzer::imageAnalysisFinished,
                    &tempContext, [&analyzer, &result, &analysisCompleted](bool success, const QString &message) {
        analysisCompleted = true;
        
        if (success) {
            // 获取实际的分析结果
            QList<FrameAnalysisResult> results = analyzer.getResults();
            if (!results.isEmpty() && results.first().success) {
                result = results.first().description;
                qDebug() << "成功获取分析结果:" << result.left(100) + "...";
            } else {
                qWarning() << "分析结果为空或失败";
                result = message; // 回退到消息
            }
        } else {
            qWarning() << "AI分析失败:" << message;
        }
    });
    
    // 启动分析
    analyzer.analyzeImages(singleImageList);
    
    // 等待分析完成（设置超时）
    int timeoutMs = 30000; // 30秒超时
    int elapsedMs = 0;
    const int checkIntervalMs = 100;
    
    while (!analysisCompleted && elapsedMs < timeoutMs) {
        QCoreApplication::processEvents();
        QThread::msleep(checkIntervalMs);
        elapsedMs += checkIntervalMs;
    }
    
    if (!analysisCompleted) {
        qWarning() << "AI分析超时:" << imagePath;
        return QString("分析超时");
    }
    
    if (result.isEmpty()) {
        return QString("分析失败");
    }
    
    return result;
}

void RealTimeAIVisionAnalyzer::generateFinalSummary() {
    if (completedAnalyses.isEmpty()) {
        emit finalSummaryGenerated(false, "", "没有可用的分析数据生成总结");
        return;
    }
    
    qDebug() << QString("开始生成最终总结，共有 %1 个分析结果").arg(completedAnalyses.size());
    
    // 构建最终总结的输入数据
    QStringList analysisTexts;
    QStringList timestamps;
    
    for (const FrameAnalysisTask &task : completedAnalyses) {
        if (task.processed && !task.analysis.isEmpty()) {
            analysisTexts << task.analysis;
            timestamps << QString::number(task.timestamp, 'f', 1) + "s";
        }
    }
    
    if (analysisTexts.isEmpty()) {
        emit finalSummaryGenerated(false, "", "没有有效的分析结果");
        return;
    }
    
    // 发出进度信号
    emit postRecordingProgress(0, 100);
    
    // 清理之前的总结分析器（如果存在）
    if (summaryAnalyzer) {
        summaryAnalyzer->cancelAnalysis();
        summaryAnalyzer->deleteLater();
        summaryAnalyzer = nullptr;
    }
    
    // 创建用于最终总结的AI分析器作为成员变量
    summaryAnalyzer = new AIVisionAnalyzer(this);
    summaryAnalyzer->setConfig(config);
    
    // 构建总结提示词
    QString summaryPrompt = "基于以下按时间顺序的屏幕内容分析，生成一份完整的视频内容总结：\n\n";
    
    for (int i = 0; i < analysisTexts.size(); ++i) {
        summaryPrompt += QString("[%1] %2\n\n").arg(timestamps[i]).arg(analysisTexts[i]);
    }
    
    summaryPrompt += "\n请提供：\n1. 视频的主要内容概述\n2. 关键活动和操作的时间线\n3. 重要信息点总结";
    
    emit postRecordingProgress(50, 100);
    
    // 调用AI模型生成真正的总结
    qDebug() << "准备调用AI模型生成最终总结";
    
    // 使用AIVisionAnalyzer进行总结生成
    connect(summaryAnalyzer, &AIVisionAnalyzer::finalSummaryGenerated,
            this, [this, analysisTexts, timestamps](bool success, const QString &summary, const QString &message) {
        qDebug() << "AI总结生成完成:" << (success ? "成功" : "失败");
        qDebug() << "总结内容长度:" << summary.length();
        qDebug() << "消息:" << message;
        
        QString finalSummary;
        if (success && !summary.isEmpty()) {
            // 直接使用AI生成的总结，不添加元数据
            finalSummary = summary;
        } else {
            // 如果AI总结失败，回退到简化版本
            qWarning() << "AI总结生成失败，回退到简化版本";
            finalSummary = QString("## 视频内容总结\n\n"
                                  "由于AI总结生成失败(%1)，以下是按时间顺序的关键帧分析：\n\n")
                            .arg(message);
            
            // 添加所有分析结果
            for (int i = 0; i < analysisTexts.size(); ++i) {
                finalSummary += QString("**[%1]** %2\n\n")
                               .arg(timestamps[i])
                               .arg(analysisTexts[i]);
            }
            
            finalSummary += "*注：AI总结生成失败，显示原始分析结果。可能原因：网络问题、API限制或模型错误。*";
        }
        
        emit postRecordingProgress(100, 100);
        emit finalSummaryGenerated(true, finalSummary, 
                                 QString("成功分析了 %1 帧并生成总结").arg(analysisTexts.size()));
                                 
        // 清理总结分析器
        if (summaryAnalyzer) {
            summaryAnalyzer->deleteLater();
            summaryAnalyzer = nullptr;
        }
    });
    
    // 调用AI生成最终总结 - 这里将所有分析结果作为描述列表传递
    summaryAnalyzer->generateFinalSummary(analysisTexts);
}

bool RealTimeAIVisionAnalyzer::isConfigValid() const {
    return config.isValid();
}
