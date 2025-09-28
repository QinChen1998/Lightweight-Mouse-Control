#include "mouserecorder.h"
#include <QDateTime>
#include <QDebug>
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
    connect(m_recordingTimer, &QTimer::timeout, this, &MouseRecorder::recordCurrentPosition);
    m_recordingTimer->setInterval(m_recordingInterval);
}

// 开始录制：清空录制缓存并启动定时器
void MouseRecorder::startRecording()
{
    if (m_isRecording) {
        qDebug() << "Already recording, ignoring start request";
        return;
    }

    qDebug() << "Starting mouse recording with interval:" << m_recordingInterval << "ms";
    m_recordedPath.clear();
    m_isRecording = true;
    m_recordingTimer->start();

    qDebug() << "Recording timer started, isActive:" << m_recordingTimer->isActive();
    emit recordingStarted();
}

// 停止录制：停止定时器并发出停止信号
void MouseRecorder::stopRecording()
{
    if (!m_isRecording) {
        qDebug() << "Not recording, ignoring stop request";
        return;
    }

    qDebug() << "Stopping mouse recording. Recorded points:" << m_recordedPath.size();
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
        qDebug() << "recordCurrentPosition called but not recording, ignoring";
        return;
    }

    QPoint currentPos;

#ifdef Q_OS_WIN
    // Use Windows API to get cursor position for consistency with playback
    POINT winPos;
    if (GetCursorPos(&winPos)) {
        currentPos = QPoint(winPos.x, winPos.y);
        // qDebug() << "Recording Windows API position:" << currentPos;
    } else {
        // Fallback to Qt if Windows API fails
        currentPos = QCursor::pos();
        qDebug() << "Fallback to Qt position:" << currentPos;
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
}
