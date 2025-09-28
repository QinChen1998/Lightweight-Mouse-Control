#include "mouseplayer.h"
#include <QDateTime>
#include <QDebug>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 鼠标播放器构造函数：初始化定时器和播放参数
MousePlayer::MousePlayer(QObject *parent)
    : QObject(parent)
    , m_playbackTimer(new QTimer(this))
    , m_currentIndex(0)
    , m_isPlaying(false)
    , m_playbackSpeed(1.0)  // 默认原始速度
    , m_startTime(0)
{
    connect(m_playbackTimer, &QTimer::timeout, this, &MousePlayer::playNextPoint);
    m_playbackTimer->setSingleShot(true);
}

// 播放鼠标路径：按照时间间隔重放鼠标移动
void MousePlayer::playPath(const QList<MousePoint>& path)
{
    if (m_isPlaying || path.isEmpty()) {
        return;
    }

    m_playbackPath = path;
    m_currentIndex = 0;
    m_isPlaying = true;
    m_startTime = QDateTime::currentMSecsSinceEpoch();

    emit playbackStarted();

    // Move to first position immediately
    moveMouseTo(m_playbackPath.first().position());
    emit positionChanged(m_playbackPath.first().position());

    if (m_playbackPath.size() > 1) {
        m_currentIndex = 1;
        // Calculate delay to next point
        qint64 delay = (m_playbackPath[1].timestamp().toMSecsSinceEpoch() -
                       m_playbackPath[0].timestamp().toMSecsSinceEpoch()) / m_playbackSpeed;
        m_playbackTimer->start(qMax(1LL, delay));
    } else {
        // Only one point, finish immediately
        m_isPlaying = false;
        emit playbackFinished();
    }
}

// 停止播放：终止定时器并重置状态
void MousePlayer::stopPlaying()
{
    if (!m_isPlaying) {
        return;
    }

    m_playbackTimer->stop();
    m_isPlaying = false;
    m_currentIndex = 0;

    emit playbackStopped();
}

// 返回当前是否正在播放
bool MousePlayer::isPlaying() const
{
    return m_isPlaying;
}

// 设置播放速度：调整时间间隔来改变播放速度
void MousePlayer::setPlaybackSpeed(double speed)
{
    m_playbackSpeed = qMax(0.1, speed); // Minimum speed of 0.1x
}

// 获取当前播放速度
double MousePlayer::playbackSpeed() const
{
    return m_playbackSpeed;
}

// 播放下一个点：移动鼠标并计算下一次延迟
void MousePlayer::playNextPoint()
{
    if (!m_isPlaying || m_currentIndex >= m_playbackPath.size()) {
        m_isPlaying = false;
        emit playbackFinished();
        return;
    }

    // Move mouse to current position
    const MousePoint& currentPoint = m_playbackPath[m_currentIndex];
    moveMouseTo(currentPoint.position());
    emit positionChanged(currentPoint.position());

    m_currentIndex++;

    // Schedule next point if available
    if (m_currentIndex < m_playbackPath.size()) {
        qint64 originalDelay = (m_playbackPath[m_currentIndex].timestamp().toMSecsSinceEpoch() -
                               currentPoint.timestamp().toMSecsSinceEpoch());
        qint64 adjustedDelay = originalDelay / m_playbackSpeed;
        qint64 finalDelay = qMax(1LL, adjustedDelay);

        // qDebug() << "Scheduling next point" << m_currentIndex
        //          << "Original delay:" << originalDelay << "ms"
        //          << "Adjusted delay:" << adjustedDelay << "ms"
        //          << "Final delay:" << finalDelay << "ms"
        //          << "Speed:" << m_playbackSpeed;

        m_playbackTimer->start(finalDelay);
    } else {
        // Finished playing all points
        m_isPlaying = false;
        emit playbackFinished();
    }
}

// 移动鼠标到指定位置：使用Windows API实现高精度移动
void MousePlayer::moveMouseTo(const QPoint& position)
{
#ifdef Q_OS_WIN
    // Use Windows API for more precise mouse movement
    BOOL result = SetCursorPos(position.x(), position.y());
    if (!result) {
        qDebug() << "SetCursorPos failed for position:" << position;
    }

    // Verify actual position after movement (for debugging)
    POINT actualPos;
    if (GetCursorPos(&actualPos)) {
        QPoint actual(actualPos.x, actualPos.y);
        if (actual != position) {
            qDebug() << "Mouse position mismatch! Target:" << position << "Actual:" << actual;
        }
    }
#else
    // Fallback for other platforms
    QCursor::setPos(position);
#endif
}
