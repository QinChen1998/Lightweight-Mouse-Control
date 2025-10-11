#ifndef COMPACTWINDOW_H
#define COMPACTWINDOW_H

#include <QWidget>
#include "mouserecorder.h"
#include "mouseplayer.h"
#include "pathmanager.h"
#include "hotkeymanager.h"
#include "QtAwesome.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class CompactWindow;
}
QT_END_NAMESPACE

class CompactWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CompactWindow(QWidget *parent = nullptr);
    ~CompactWindow();

    // Set references to shared components from MainWindow
    void setComponents(MouseRecorder *recorder,
                      MousePlayer *player,
                      PathManager *pathManager,
                      HotkeyManager *hotkeyManager);

    // Set playback parameters
    void setPlaybackSpeed(double speed);
    void setRepeatCount(int count);

    // Get recording hotkey from settings
    void setRecordingHotkey(const QString& hotkey);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    // Recording controls
    void onRecordButtonClicked();
    void onRecordingStarted();
    void onRecordingStopped();
    void onPointRecorded(const MousePoint& point);

    // Playback controls
    void onPlayButtonClicked();
    void onStopButtonClicked();
    void onPlaybackStarted();
    void onPlaybackFinished();
    void onPlaybackStopped();

    // Path management
    void onPathSaved(const QString& filename);

    // Hotkey handling
    void onRecordingHotkeyPressed();
    void onStopPlaybackHotkeyPressed();

private:
    void connectSignals();
    void updateRecordButton();
    void updatePlayButton();
    void updateStatus(const QString& status);

    Ui::CompactWindow *ui;

    // Icon library
    fa::QtAwesome *m_awesome;

    // Shared components (not owned)
    MouseRecorder *m_recorder;
    MousePlayer *m_player;
    PathManager *m_pathManager;
    HotkeyManager *m_hotkeyManager;

    // Playback settings
    double m_playbackSpeed;
    int m_repeatCount;

    // State tracking
    int m_recordedPointsCount;
    QString m_lastSavedPath;
    QList<MousePoint> m_currentPlaybackPath;
    int m_remainingRepeats;
    int m_totalRepeats;
    QString m_recordingHotkey;
};

#endif // COMPACTWINDOW_H
