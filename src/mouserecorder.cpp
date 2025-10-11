#include "mouserecorder.h"
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 鼠标录制器构造函数：初始化定时器和录制参数
MouseRecorder::MouseRecorder(QObject *parent)
    : QObject(parent)
    , m_recordingTimer(new QTimer(this))
    , m_isRecording(false)
    , m_recordingInterval(50) // 50ms间隔，保证平滑录制
{
    connect(m_recordingTimer, &QTimer::timeout, this, &MouseRecorder::recordCurrentPosition, Qt::DirectConnection);

    // Set timer type before setting interval
    m_recordingTimer->setTimerType(Qt::PreciseTimer);
    m_recordingTimer->setInterval(m_recordingInterval);
}

// 开始录制：清空录制缓存并启动定时器
void MouseRecorder::startRecording()
{
    if (m_isRecording) {
        return;
    }

    m_recordedPath.clear();
    m_isRecording = true;

    // Stop timer first to ensure clean state
    m_recordingTimer->stop();

    // Set timer properties
    m_recordingTimer->setTimerType(Qt::PreciseTimer);
    m_recordingTimer->setInterval(m_recordingInterval);

    // Start the timer
    m_recordingTimer->start();

    // Force event processing
    QCoreApplication::processEvents();

    emit recordingStarted();
}

// 停止录制：停止定时器并发出停止信号
void MouseRecorder::stopRecording()
{
    if (!m_isRecording) {
        return;
    }

    m_recordingTimer->stop();
    m_isRecording = false;

    emit recordingStopped();
}

// 返回当前是否正在录制
bool MouseRecorder::isRecording() const
{
    return m_isRecording;
}

// 获取录制的鼠标路径数据
QList<MousePoint> MouseRecorder::getRecordedPath() const
{
    return m_recordedPath;
}

// 清空录制的路径数据
void MouseRecorder::clearRecordedPath()
{
    m_recordedPath.clear();
}

// 设置录制间隔：调整定时器的采样频率
void MouseRecorder::setRecordingInterval(int intervalMs)
{
    m_recordingInterval = intervalMs;
    m_recordingTimer->setInterval(intervalMs);
}

// 获取当前录制间隔设置
int MouseRecorder::recordingInterval() const
{
    return m_recordingInterval;
}

// 录制当前鼠标位置：使用Windows API获取高精度坐标
void MouseRecorder::recordCurrentPosition()
{
    if (!m_isRecording) {
        return;
    }

    QPoint currentPos;

#ifdef Q_OS_WIN
    // Use Windows API to get cursor position for consistency with playback
    POINT winPos;
    if (GetCursorPos(&winPos)) {
        currentPos = QPoint(winPos.x, winPos.y);
    } else {
        // Fallback to Qt if Windows API fails
        currentPos = QCursor::pos();
    }
#else
    currentPos = QCursor::pos();
#endif

    // Use high precision timestamp for better playback accuracy
    QDateTime timestamp = QDateTime::currentDateTime();
    timestamp.setMSecsSinceEpoch(QDateTime::currentMSecsSinceEpoch());

    // qDebug() << "Recording point" << m_recordedPath.size() + 1 << "at position:" << currentPos
    //          << "timestamp:" << timestamp.toString("hh:mm:ss.zzz");

    MousePoint point(currentPos, timestamp);
    m_recordedPath.append(point);

    emit pointRecorded(point);

    // Check if we've reached the maximum recording limit
    if (m_recordedPath.size() >= MAX_RECORDING_POINTS) {
        qWarning() << "Recording limit reached (" << MAX_RECORDING_POINTS << " points). Stopping recording.";
        emit recordingLimitReached();
        stopRecording();
    }
}
