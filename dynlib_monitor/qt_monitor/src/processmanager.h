#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include <QProcess>
#include "eventdata.h"

/**
 * @brief 进程管理器类
 * 
 * 该类负责管理和控制后端监控进程的生命周期，包括：
 * 1. 启动和停止后端监控进程
 * 2. 捕获和处理后端进程的输出
 * 3. 解析输出内容并转发给事件数据管理器
 */
class ProcessManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param eventData 事件数据管理器指针
     * @param parent 父对象指针（Qt对象树机制）
     */
    explicit ProcessManager(EventData* eventData, QObject *parent = nullptr);
    
    /**
     * @brief 析构函数，负责清理资源
     */
    ~ProcessManager();

    /**
     * @brief 启动监控进程
     * @param targetProcess 要监控的目标进程名称
     * @return 启动是否成功
     */
    bool startMonitor(const QString& targetProcess = QString());
    
    /**
     * @brief 停止监控进程
     */
    void stopMonitor();
    
    /**
     * @brief 检查监控进程是否正在运行
     * @return 如果监控进程正在运行返回true，否则返回false
     */
    bool isRunning() const;

signals:
    /**
     * @brief 监控进程启动成功时发出的信号
     */
    void monitorStarted();
    
    /**
     * @brief 监控进程停止时发出的信号
     */
    void monitorStopped();
    
    /**
     * @brief 发生错误时发出的信号
     * @param error 错误信息
     */
    void errorOccurred(const QString& error);

private slots:
    /**
     * @brief 处理监控进程的标准输出
     */
    void handleProcessOutput();
    
    /**
     * @brief 处理监控进程的错误
     * @param error 错误类型
     */
    void handleProcessError(QProcess::ProcessError error);
    
    /**
     * @brief 处理监控进程结束
     * @param exitCode 退出码
     * @param exitStatus 退出状态
     */
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess* process;      ///< 用于管理后端监控进程的QProcess对象
    EventData* eventData;   ///< 事件数据管理器指针
    QString currentBuffer;  ///< 当前输出缓冲区
    
    /**
     * @brief 处理并解析事件文本
     * 将进程输出的文本解析为事件，并发送给事件数据管理器
     */
    void processEventText();
};

#endif // PROCESSMANAGER_H 