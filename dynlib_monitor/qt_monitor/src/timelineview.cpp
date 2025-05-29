#include "timelineview.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QStringList>

TimelineView::TimelineView(EventData* eventData, QWidget *parent)
    : QWidget(parent)
    , eventData(eventData)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // 创建表格
    eventTable = new QTableWidget(this);
    setupTable();
    
    layout->addWidget(eventTable);
    
    // 设置大小
    resize(1200, 800);
}

void TimelineView::setupTable() {
    // 设置列
    QStringList headers;
    headers << "时间戳" << "事件类型" << "进程名" << "进程ID" << "详细信息";
    eventTable->setColumnCount(headers.size());
    eventTable->setHorizontalHeaderLabels(headers);
    
    // 设置表格属性
    eventTable->setEditTriggers(QTableWidget::NoEditTriggers);
    eventTable->setSelectionBehavior(QTableWidget::SelectRows);
    eventTable->setSelectionMode(QTableWidget::SingleSelection);
    eventTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    eventTable->verticalHeader()->setVisible(false);
    eventTable->setSortingEnabled(true);
    
    // 设置自动调整行高以适应内容
    eventTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void TimelineView::addEvent(const Event& event) {
    // 在表格开头插入新行
    eventTable->insertRow(0);
    
    // 添加事件信息
    eventTable->setItem(0, 0, new QTableWidgetItem(event.timestamp));
    eventTable->setItem(0, 1, new QTableWidgetItem(event.eventType));
    
    // 从详细信息中提取进程信息
    QString processName = event.details.value("进程名", "");
    QString processId = event.details.value("进程ID", "");
    
    eventTable->setItem(0, 2, new QTableWidgetItem(processName));
    eventTable->setItem(0, 3, new QTableWidgetItem(processId));
    
    // 构建详细信息字符串
    QString details;
    for (auto it = event.details.constBegin(); it != event.details.constEnd(); ++it) {
        if (it.key() != "进程名" && it.key() != "进程ID") {
            if (!details.isEmpty()) {
                details += "\n";
            }
            details += QString("%1: %2").arg(it.key(), it.value());
        }
    }
    
    // 创建详细信息单元格并设置属性
    QTableWidgetItem* detailsItem = new QTableWidgetItem(details);
    detailsItem->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
    detailsItem->setFlags(detailsItem->flags() | Qt::ItemIsEnabled);  // 确保单元格是启用的
    eventTable->setItem(0, 4, detailsItem);
    
    // 调整列宽以适应内容
    eventTable->resizeColumnsToContents();
    
    // 设置单元格自动换行
    eventTable->setWordWrap(true);
}

void TimelineView::clear() {
    eventTable->setRowCount(0);
}