#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QTableWidget>
#include "eventdata.h"

/**
 * @brief 时间线视图类
 * 
 * 该类负责以表格形式显示动态链接事件的时间线，包括：
 * 1. 显示事件的时间戳、类型和详细信息
 * 2. 实时更新新的事件
 * 3. 提供清除和重置视图的功能
 */
class TimelineView : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param eventData 事件数据管理器指针
     * @param parent 父窗口部件指针
     */
    explicit TimelineView(EventData* eventData, QWidget *parent = nullptr);

    /**
     * @brief 添加新事件到时间线视图
     * @param event 要添加的事件
     */
    void addEvent(const Event& event);

    /**
     * @brief 清除时间线视图中的所有事件
     */
    void clear();

private:
    EventData* eventData;      ///< 事件数据管理器指针
    QTableWidget* eventTable;  ///< 用于显示事件的表格部件
    
    /**
     * @brief 初始化表格的列和样式
     */
    void setupTable();

    /**
     * @brief 更新事件的详细信息显示
     * @param event 要更新显示的事件
     */
    void updateEventDetails(const Event& event);
};

#endif // TIMELINEVIEW_H 