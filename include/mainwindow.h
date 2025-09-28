#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QMessageBox>
#include "mouserecorder.h"
#include "mouseplayer.h"
#include "pathmanager.h"
#include "hotkeymanager.h"
#include "settingsdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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
    void onSpeedChanged(double speed);

    // Path management
    void onPathSelectionChanged();
    void onDeletePathClicked();
    void onBatchDeleteClicked();
    void onSelectAllClicked();
    void onRefreshPathsClicked();
    void onRenamePathClicked();
    void onPathSaved(const QString& filename);
    void onPathDeleted(const QString& filename);
    void onPathsDeleted(const QStringList& filenames);
    void onPathRenamed(const QString& oldFilename, const QString& newFilename);

    // Menu actions
    void onActionExit();
    void onActionAbout();
    void onActionDiagnostics();

    // Settings
    void onSettingsClicked();

    // Hotkey handling
    void onRecordingHotkeyPressed();
    void registerHotkeysWhenReady();

    // Settings handling
    void onSettingsChanged();

private:
    void setupUI();
    void connectSignals();
    void updateUI();
    void updatePathList();
    void updatePathDetails();
    void updateRecordingStatus(const QString& status);
    void updateIntervalDisplay();
    void updateRecordButtonText();
    void loadSettings();
    void applySettings();

protected:
    void showEvent(QShowEvent *event) override;

    Ui::MainWindow *ui;
    MouseRecorder *m_recorder;
    MousePlayer *m_player;
    PathManager *m_pathManager;
    HotkeyManager *m_hotkeyManager;
    SettingsDialog *m_settingsDialog;

    // UI state
    int m_recordedPointsCount;
    QString m_currentSelectedPath;
    bool m_hotkeysRegistered;
};
#endif // MAINWINDOW_H
