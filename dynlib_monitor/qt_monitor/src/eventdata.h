#ifndef EVENTDATA_H
#define EVENTDATA_H

#include <QString>
#include <QVector>
#include <QObject>
#include <QMap>

/**
 * @brief 事件数据结构
 * 用于存储单个动态链接事件的详细信息
 */
struct Event {
    QString timestamp;    ///< 事件发生的时间戳
    QString eventType;    ///< 事件类型（加载/卸载/符号解析）
    QMap<QString, QString> details;  ///< 事件的详细信息，以键值对形式存储
};

/**
 * @brief 事件数据管理类
 * 
 * 该类负责管理和存储所有动态链接相关的事件，包括：
 * 1. 解析和存储新的事件
 * 2. 提供事件查询接口
 * 3. 按事件类型分类和过滤事件
 */
class EventData : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针（Qt对象树机制）
     */
    explicit EventData(QObject *parent = nullptr);

    /**
     * @brief 添加新的事件
     * @param eventText 事件的原始文本描述
     */
    void addEvent(const QString& eventText);

    /**
     * @brief 获取所有已记录的事件
     * @return 事件列表的常量引用
     */
    const QVector<Event>& getAllEvents() const { return events; }

    /**
     * @brief 获取所有动态库加载事件
     * @return 加载事件列表
     */
    QVector<Event> getLoadEvents() const;

    /**
     * @brief 获取所有符号解析事件
     * @return 符号解析事件列表
     */
    QVector<Event> getSymbolEvents() const;

    /**
     * @brief 获取所有动态库卸载事件
     * @return 卸载事件列表
     */
    QVector<Event> getUnloadEvents() const;

signals:
    /**
     * @brief 当新事件被添加时发出的信号
     * @param event 新添加的事件
     */
    void eventAdded(const Event& event);

private:
    QVector<Event> events;  ///< 存储所有事件的容器

    /**
     * @brief 解析事件文本
     * @param eventText 原始事件文本
     * @return 解析后的Event对象
     */
    Event parseEventText(const QString& eventText);
};

#endif // EVENTDATA_H 