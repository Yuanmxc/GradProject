#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include <QProcess>
#include "eventdata.h"

class ProcessManager : public QObject {
    Q_OBJECT

public:
    explicit ProcessManager(EventData* eventData, QObject *parent = nullptr);
    ~ProcessManager();

    bool startMonitor(const QString& targetProcess = QString());
    void stopMonitor();
    bool isRunning() const;

signals:
    void monitorStarted();
    void monitorStopped();
    void errorOccurred(const QString& error);

private slots:
    void handleProcessOutput();
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess* process;
    EventData* eventData;
    QString currentBuffer;
    
    void processEventText();
};

#endif // PROCESSMANAGER_H 