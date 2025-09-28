#ifndef MOUSEPLAYER_H
#define MOUSEPLAYER_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QCursor>
#include "mousepoint.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class MousePlayer : public QObject
{
    Q_OBJECT

public:
    explicit MousePlayer(QObject *parent = nullptr);

    void playPath(const QList<MousePoint>& path);
    void stopPlaying();
    bool isPlaying() const;

    void setPlaybackSpeed(double speed); // 1.0 = normal speed, 0.5 = half speed, 2.0 = double speed
    double playbackSpeed() const;

signals:
    void playbackStarted();
    void playbackFinished();
    void playbackStopped();
    void positionChanged(const QPoint& position);

private slots:
    void playNextPoint();

private:
    void moveMouseTo(const QPoint& position);

    QTimer *m_playbackTimer;
    QList<MousePoint> m_playbackPath;
    int m_currentIndex;
    bool m_isPlaying;
    double m_playbackSpeed;
    qint64 m_startTime;
};

#endif // MOUSEPLAYER_H