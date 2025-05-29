#include "eventdata.h"
#include <QRegularExpression>
#include <QDebug>

EventData::EventData(QObject *parent) : QObject(parent) {}

void EventData::addEvent(const QString& eventText) {
    Event event = parseEventText(eventText);
    if (!event.timestamp.isEmpty()) {
        events.append(event);
        emit eventAdded(event);
    } else {
        qDebug() << "无效的事件时间戳:" << eventText;
    }
}

Event EventData::parseEventText(const QString& eventText) {
    Event event;
    QStringList lines = eventText.split('\n', Qt::SkipEmptyParts);
    
    if (lines.isEmpty()) {
        return event;
    }
    
    // 解析第一行获取时间戳和事件类型
    QString firstLine = lines[0];
    QRegularExpression re("\\[(.*?)\\]\\s*事件：(.*)");
    QRegularExpressionMatch match = re.match(firstLine);
    
    if (match.hasMatch()) {
        event.timestamp = match.captured(1);  // 直接使用原始时间戳字符串
        event.eventType = match.captured(2).trimmed();
        
        // 调试输出
        qDebug() << "解析时间戳:" << event.timestamp;
    } else {
        qDebug() << "无法匹配时间戳格式:" << firstLine;
    }

    // 解析剩余行获取详细信息
    for (int i = 1; i < lines.size(); ++i) {
        QString line = lines[i];
        int colonPos = line.indexOf(':');
        if (colonPos != -1) {
            QString key = line.left(colonPos).trimmed();
            QString value = line.mid(colonPos + 1).trimmed();
            event.details[key] = value;
        }
    }

    return event;
}

QVector<Event> EventData::getLoadEvents() const {
    QVector<Event> loadEvents;
    for (const Event& event : events) {
        if (event.eventType == "动态库加载事件") {
            loadEvents.append(event);
        }
    }
    return loadEvents;
}

QVector<Event> EventData::getSymbolEvents() const {
    QVector<Event> symbolEvents;
    for (const Event& event : events) {
        if (event.eventType == "符号解析事件") {
            symbolEvents.append(event);
        }
    }
    return symbolEvents;
}

QVector<Event> EventData::getUnloadEvents() const {
    QVector<Event> unloadEvents;
    for (const Event& event : events) {
        if (event.eventType == "动态库卸载事件") {
            unloadEvents.append(event);
        }
    }
    return unloadEvents;
} 