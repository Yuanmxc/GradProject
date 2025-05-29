#include "processmanager.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QProcessEnvironment>

ProcessManager::ProcessManager(EventData* eventData, QObject *parent)
    : QObject(parent)
    , process(new QProcess(this))
    , eventData(eventData)
{
    // 设置进程通道，将标准输出和标准错误都重定向到我们的程序
    process->setProcessChannelMode(QProcess::MergedChannels);
    
    // 设置环境变量，禁用输出缓冲
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONUNBUFFERED", "1");  // 如果后端是Python程序
    env.insert("GLIBC_UNBUFFERED", "1");  // 通用的unbuffered设置
    process->setProcessEnvironment(env);
    
    connect(process, &QProcess::readyReadStandardOutput, this, &ProcessManager::handleProcessOutput);
    connect(process, &QProcess::errorOccurred, this, &ProcessManager::handleProcessError);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::handleProcessFinished);
}

ProcessManager::~ProcessManager() {
    stopMonitor();
    delete process;
}

bool ProcessManager::startMonitor(const QString& targetProcess) {
    if (process->state() != QProcess::NotRunning) {
        return false;
    }

    QStringList arguments;
    
    // 添加dynlib_monitor的完整路径
    QString monitorPath = "/home/Yuanmxc/Course/GradProject/dynlib_monitor/build/dynlib_monitor";

    // 处理目标进程
    if (!targetProcess.isEmpty()) {
        arguments << monitorPath << targetProcess;
        qDebug() << "监控进程:" << targetProcess;
    } else {
        arguments << monitorPath;
    }

    // 启动进程
    qDebug() << "启动命令: pkexec" << arguments.join(" ");
    process->start("pkexec", arguments);
    
    if (!process->waitForStarted()) {
        emit errorOccurred("无法启动监控进程");
        return false;
    }

    emit monitorStarted();
    return true;
}

void ProcessManager::stopMonitor() {
    if (process->state() != QProcess::NotRunning) {
        // 发送SIGINT信号（Ctrl+C）
        process->terminate();
        if (!process->waitForFinished(3000)) {
            process->kill();
        }
    }
}

bool ProcessManager::isRunning() const {
    return process->state() != QProcess::NotRunning;
}

void ProcessManager::handleProcessOutput() {
    // 读取新的输出
    QByteArray newData = process->readAllStandardOutput();
    QString output = QString::fromUtf8(newData);
    
    // 打印到终端
    qDebug().noquote() << output;
    
    // 添加到缓冲区并处理
    currentBuffer += output;
    
    // 立即处理事件
    processEventText();
    
    // 强制更新UI
    QCoreApplication::processEvents();
}

void ProcessManager::handleProcessError(QProcess::ProcessError error) {
    QString errorMessage;
    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "进程启动失败";
            break;
        case QProcess::Crashed:
            errorMessage = "进程崩溃";
            break;
        default:
            errorMessage = "发生未知错误";
            break;
    }
    emit errorOccurred(errorMessage);
}

void ProcessManager::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::CrashExit) {
        emit errorOccurred("进程异常终止");
    }
    emit monitorStopped();
}

void ProcessManager::processEventText() {
    int startPos = 0;
    while (true) {
        // 查找事件开始标记
        int nextStart = currentBuffer.indexOf("[", startPos + 1);
        
        // 如果找不到下一个开始标记，但缓冲区中有内容
        if (nextStart == -1) {
            QString remainingText = currentBuffer.mid(startPos).trimmed();
            if (!remainingText.isEmpty() && remainingText.contains("] 事件：")) {
                eventData->addEvent(remainingText);
                currentBuffer.clear();
            }
            break;
        }

        // 提取当前事件文本
        QString eventText = currentBuffer.mid(startPos, nextStart - startPos).trimmed();
        
        // 检查是否是完整的事件（包含时间戳和事件类型）
        if (!eventText.isEmpty() && eventText.contains("] 事件：")) {
            // 立即发送事件到EventData处理
            eventData->addEvent(eventText);
        }
        
        startPos = nextStart;
    }

    // 保留最后一个不完整的事件
    if (startPos > 0 && startPos < currentBuffer.length()) {
        currentBuffer = currentBuffer.mid(startPos);
    }
} 