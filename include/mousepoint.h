#ifndef MOUSEPOINT_H
#define MOUSEPOINT_H

#include <QPoint>
#include <QDateTime>
#include <QDataStream>

class MousePoint
{
public:
    MousePoint();
    MousePoint(const QPoint& position, const QDateTime& timestamp);
    MousePoint(int x, int y, const QDateTime& timestamp);
    MousePoint(const QPoint& position, quint32 deltaMs);
    MousePoint(int x, int y, quint32 deltaMs);

    QPoint position() const;
    QDateTime timestamp() const;
    qint64 delay() const;
    quint32 deltaMs() const;
    bool isRelativeTime() const;

    void setPosition(const QPoint& position);
    void setTimestamp(const QDateTime& timestamp);
    void setDeltaMs(quint32 deltaMs);

    friend QDataStream& operator<<(QDataStream& out, const MousePoint& point);
    friend QDataStream& operator>>(QDataStream& in, MousePoint& point);

private:
    QPoint m_position;
    QDateTime m_timestamp;
    quint32 m_deltaMs;
    bool m_useRelativeTime;
};

#endif // MOUSEPOINT_H