#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , eventData(new EventData(this))
    , processManager(new ProcessManager(eventData, this))
{
    setupUI();
    
    connect(processManager, &ProcessManager::monitorStarted, this, &MainWindow::handleMonitorStarted);
    connect(processManager, &ProcessManager::monitorStopped, this, &MainWindow::handleMonitorStopped);
    connect(processManager, &ProcessManager::errorOccurred, this, &MainWindow::handleError);
    connect(eventData, &EventData::eventAdded, this, &MainWindow::handleNewEvent);
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // 控制面板
    QHBoxLayout* controlLayout = new QHBoxLayout();
    QLabel* label = new QLabel("目标进程名称（可选）：");
    targetProcessEdit = new QLineEdit();
    startButton = new QPushButton("开始监控");
    stopButton = new QPushButton("停止监控");
    statusLabel = new QLabel("就绪");
    
    controlLayout->addWidget(label);
    controlLayout->addWidget(targetProcessEdit);
    controlLayout->addWidget(startButton);
    controlLayout->addWidget(stopButton);
    controlLayout->addWidget(statusLabel);
    
    mainLayout->addLayout(controlLayout);
    
    // 标签页
    tabWidget = new QTabWidget();
    timelineView = new TimelineView(eventData);
    
    tabWidget->addTab(timelineView, "时间线视图");
    
    setupEventTables();
    
    mainLayout->addWidget(tabWidget);
    
    // 连接按钮信号
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startMonitoring);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopMonitoring);
    
    // 初始状态
    stopButton->setEnabled(false);
    resize(1200, 800);
}

void MainWindow::setupEventTables() {
    loadTable = new QTableWidget();
    symbolTable = new QTableWidget();
    unloadTable = new QTableWidget();
    
    QStringList headers;
    headers << "时间戳" << "进程名" << "进程ID" << "库路径" << "加载基址" << "标志";
    loadTable->setColumnCount(headers.size());
    loadTable->setHorizontalHeaderLabels(headers);
    
    headers.clear();
    headers << "时间戳" << "进程名" << "进程ID" << "符号名" << "所属库" << "解析地址";
    symbolTable->setColumnCount(headers.size());
    symbolTable->setHorizontalHeaderLabels(headers);
    
    headers.clear();
    headers << "时间戳" << "进程名" << "进程ID" << "库路径" << "卸载结果";
    unloadTable->setColumnCount(headers.size());
    unloadTable->setHorizontalHeaderLabels(headers);
    
    // 设置表格属性
    auto setupTable = [](QTableWidget* table) {
        table->setEditTriggers(QTableWidget::NoEditTriggers);
        table->setSelectionBehavior(QTableWidget::SelectRows);
        table->setSelectionMode(QTableWidget::SingleSelection);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);  // 允许调整列宽
        table->verticalHeader()->setVisible(false);
        table->setSortingEnabled(true);  // 启用排序
    };
    
    setupTable(loadTable);
    setupTable(symbolTable);
    setupTable(unloadTable);
    
    tabWidget->addTab(loadTable, "加载事件");
    tabWidget->addTab(symbolTable, "符号解析事件");
    tabWidget->addTab(unloadTable, "卸载事件");
}

void MainWindow::startMonitoring() {
    QString targetProcess = targetProcessEdit->text().trimmed();
    if (processManager->startMonitor(targetProcess)) {
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        targetProcessEdit->setEnabled(false);
        statusLabel->setText("正在监控中...");
    }
}

void MainWindow::stopMonitoring() {
    processManager->stopMonitor();
}

void MainWindow::handleMonitorStarted() {
    statusLabel->setText("监控已启动");
}

void MainWindow::handleMonitorStopped() {
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    targetProcessEdit->setEnabled(true);
    statusLabel->setText("监控已停止");
}

void MainWindow::handleError(const QString& error) {
    QMessageBox::critical(this, "错误", error);
    handleMonitorStopped();
}

void MainWindow::handleNewEvent(const Event& event) {
    timelineView->addEvent(event);
    
    QTableWidget* targetTable = nullptr;
    QList<QTableWidgetItem*> items;
    
    if (event.eventType == "动态库加载事件") {
        targetTable = loadTable;
        items << new QTableWidgetItem(event.timestamp)
              << new QTableWidgetItem(event.details["进程名"])
              << new QTableWidgetItem(event.details["进程ID"])
              << new QTableWidgetItem(event.details["加载库路径"])
              << new QTableWidgetItem(event.details["加载基址"])
              << new QTableWidgetItem(event.details["标志"]);
    }
    else if (event.eventType == "符号解析事件") {
        targetTable = symbolTable;
        items << new QTableWidgetItem(event.timestamp)
              << new QTableWidgetItem(event.details["进程名"])
              << new QTableWidgetItem(event.details["进程ID"])
              << new QTableWidgetItem(event.details["请求符号"])
              << new QTableWidgetItem(event.details["所属库"])
              << new QTableWidgetItem(event.details["解析地址"]);
    }
    else if (event.eventType == "动态库卸载事件") {
        targetTable = unloadTable;
        items << new QTableWidgetItem(event.timestamp)
              << new QTableWidgetItem(event.details["进程名"])
              << new QTableWidgetItem(event.details["进程ID"])
              << new QTableWidgetItem(event.details["卸载库路径"])
              << new QTableWidgetItem(event.details["卸载结果"]);
    }
    
    if (targetTable) {
        // 设置时间戳的排序键（使用字符串本身作为排序键）
        items[0]->setData(Qt::UserRole, event.timestamp);
        
        // 在表格开头插入新行
        targetTable->insertRow(0);
        for (int i = 0; i < items.size(); ++i) {
            targetTable->setItem(0, i, items[i]);
        }
        
        // 按时间戳降序排序
        targetTable->sortItems(0, Qt::DescendingOrder);
    }
} 