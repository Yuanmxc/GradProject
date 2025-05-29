#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QTableWidget>
#include "eventdata.h"

class TimelineView : public QWidget {
    Q_OBJECT

public:
    explicit TimelineView(EventData* eventData, QWidget *parent = nullptr);
    void addEvent(const Event& event);
    void clear();

private:
    EventData* eventData;
    QTableWidget* eventTable;
    
    void setupTable();
    void updateEventDetails(const Event& event);
};

#endif // TIMELINEVIEW_H 