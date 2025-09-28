#ifndef MOUSERECORDER_H
#define MOUSERECORDER_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QCursor>
#include "mousepoint.h"

class MouseRecorder : public QObject
{
    Q_OBJECT

public:
    explicit MouseRecorder(QObject *parent = nullptr);

    void startRecording();
    void stopRecording();
    bool isRecording() const;

    QList<MousePoint> getRecordedPath() const;
    void clearRecordedPath();

    void setRecordingInterval(int intervalMs);
    int recordingInterval() const;

signals:
    void recordingStarted();
    void recordingStopped();
    void pointRecorded(const MousePoint& point);
    void recordingLimitReached();

private slots:
    void recordCurrentPosition();

private:
    QTimer *m_recordingTimer;
    QList<MousePoint> m_recordedPath;
    bool m_isRecording;
    int m_recordingInterval;

    // Maximum points limit (5 minutes at 1ms interval = 300,000 points)
    static const int MAX_RECORDING_POINTS = 300000;
};

#endif // MOUSERECORDER_H