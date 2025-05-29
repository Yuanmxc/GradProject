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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startMonitoring();
    void stopMonitoring();
    void handleMonitorStarted();
    void handleMonitorStopped();
    void handleError(const QString& error);
    void handleNewEvent(const Event& event);

private:
    void setupUI();
    void setupEventTables();
    void addEventToTable(QTableWidget* table, const Event& event);
    
    EventData* eventData;
    ProcessManager* processManager;
    TimelineView* timelineView;
    
    QTabWidget* tabWidget;
    QTableWidget* loadTable;
    QTableWidget* symbolTable;
    QTableWidget* unloadTable;
    
    QLineEdit* targetProcessEdit;
    QPushButton* startButton;
    QPushButton* stopButton;
    QLabel* statusLabel;
};

#endif // MAINWINDOW_H 