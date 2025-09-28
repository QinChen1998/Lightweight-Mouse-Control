#include "mousepoint.h"

MousePoint::MousePoint()
    : m_position(0, 0), m_timestamp(QDateTime::currentDateTime()), m_deltaMs(0), m_useRelativeTime(false)
{
}

MousePoint::MousePoint(const QPoint& position, const QDateTime& timestamp)
    : m_position(position), m_timestamp(timestamp), m_deltaMs(0), m_useRelativeTime(false)
{
}

MousePoint::MousePoint(int x, int y, const QDateTime& timestamp)
    : m_position(x, y), m_timestamp(timestamp), m_deltaMs(0), m_useRelativeTime(false)
{
}

MousePoint::MousePoint(const QPoint& position, quint32 deltaMs)
    : m_position(position), m_timestamp(), m_deltaMs(deltaMs), m_useRelativeTime(true)
{
}

MousePoint::MousePoint(int x, int y, quint32 deltaMs)
    : m_position(x, y), m_timestamp(), m_deltaMs(deltaMs), m_useRelativeTime(true)
{
}

QPoint MousePoint::position() const
{
    return m_position;
}

QDateTime MousePoint::timestamp() const
{
    return m_timestamp;
}

qint64 MousePoint::delay() const
{
    return m_timestamp.toMSecsSinceEpoch();
}

quint32 MousePoint::deltaMs() const
{
    return m_deltaMs;
}

bool MousePoint::isRelativeTime() const
{
    return m_useRelativeTime;
}

void MousePoint::setPosition(const QPoint& position)
{
    m_position = position;
}

void MousePoint::setTimestamp(const QDateTime& timestamp)
{
    m_timestamp = timestamp;
    m_useRelativeTime = false;
}

void MousePoint::setDeltaMs(quint32 deltaMs)
{
    m_deltaMs = deltaMs;
    m_useRelativeTime = true;
}

QDataStream& operator<<(QDataStream& out, const MousePoint& point)
{
    out << point.m_position << point.m_useRelativeTime;
    if (point.m_useRelativeTime) {
        out << point.m_deltaMs;
    } else {
        out << point.m_timestamp;
    }
    return out;
}

QDataStream& operator>>(QDataStream& in, MousePoint& point)
{
    in >> point.m_position >> point.m_useRelativeTime;
    if (point.m_useRelativeTime) {
        in >> point.m_deltaMs;
        point.m_timestamp = QDateTime();
    } else {
        in >> point.m_timestamp;
        point.m_deltaMs = 0;
    }
    return in;
}