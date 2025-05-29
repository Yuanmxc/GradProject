#ifndef EVENTDATA_H
#define EVENTDATA_H

#include <QString>
#include <QVector>
#include <QObject>
#include <QMap>

struct Event {
    QString timestamp;
    QString eventType;
    QMap<QString, QString> details;
};

class EventData : public QObject {
    Q_OBJECT

public:
    explicit EventData(QObject *parent = nullptr);
    void addEvent(const QString& eventText);
    const QVector<Event>& getAllEvents() const { return events; }
    QVector<Event> getLoadEvents() const;
    QVector<Event> getSymbolEvents() const;
    QVector<Event> getUnloadEvents() const;

signals:
    void eventAdded(const Event& event);

private:
    QVector<Event> events;
    Event parseEventText(const QString& eventText);
};

#endif // EVENTDATA_H 