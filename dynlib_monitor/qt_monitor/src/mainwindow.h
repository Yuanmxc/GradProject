#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "eventdata.h"
#include "timelineview.h"
#include "processmanager.h"

/**
 * @brief 主窗口类
 * 
 * 该类是应用程序的主窗口，负责：
 * 1. 创建和管理用户界面
 * 2. 处理用户交互
 * 3. 协调事件数据管理器、进程管理器和时间线视图之间的通信
 * 4. 显示不同类型事件的分类视图
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口部件指针
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~MainWindow();

private slots:
    /**
     * @brief 开始监控目标进程
     */
    void startMonitoring();
    
    /**
     * @brief 停止监控
     */
    void stopMonitoring();
    
    /**
     * @brief 处理监控开始事件
     */
    void handleMonitorStarted();
    
    /**
     * @brief 处理监控停止事件
     */
    void handleMonitorStopped();
    
    /**
     * @brief 处理错误事件
     * @param error 错误信息
     */
    void handleError(const QString& error);
    
    /**
     * @brief 处理新的动态链接事件
     * @param event 新事件
     */
    void handleNewEvent(const Event& event);

private:
    /**
     * @brief 初始化用户界面
     */
    void setupUI();
    
    /**
     * @brief 初始化事件表格
     */
    void setupEventTables();
    
    /**
     * @brief 添加事件到指定的表格
     * @param table 目标表格
     * @param event 要添加的事件
     */
    void addEventToTable(QTableWidget* table, const Event& event);
    
    // 核心组件
    EventData* eventData;          ///< 事件数据管理器
    ProcessManager* processManager; ///< 进程管理器
    TimelineView* timelineView;    ///< 时间线视图
    
    // 界面组件
    QTabWidget* tabWidget;         ///< 标签页管理器
    QTableWidget* loadTable;       ///< 加载事件表格
    QTableWidget* symbolTable;     ///< 符号解析事件表格
    QTableWidget* unloadTable;     ///< 卸载事件表格
    
    QLineEdit* targetProcessEdit;  ///< 目标进程输入框
    QPushButton* startButton;      ///< 开始监控按钮
    QPushButton* stopButton;       ///< 停止监控按钮
    QLabel* statusLabel;           ///< 状态标签
};

#endif // MAINWINDOW_H 