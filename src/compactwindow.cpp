#include "compactwindow.h"
#include "ui_compactwindow.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QShowEvent>

// 紧凑窗口构造函数：初始化UI和状态
CompactWindow::CompactWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CompactWindow)
    , m_awesome(new fa::QtAwesome(this))
    , m_recorder(nullptr)
    , m_player(nullptr)
    , m_pathManager(nullptr)
    , m_hotkeyManager(nullptr)
    , m_playbackSpeed(1.0)
    , m_repeatCount(1)
    , m_recordedPointsCount(0)
    , m_remainingRepeats(0)
    , m_totalRepeats(1)
    , m_recordingHotkey("Ctrl+B")
{
    ui->setupUi(this);

    // Initialize QtAwesome
    m_awesome->initFontAwesome();

    // Set window flags to stay on top
    // Use Qt::Window to ensure proper event handling
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);

    // Set button icons with colors
    QVariantMap recordOptions;
    recordOptions.insert("color", QColor(231, 76, 60));  // Red
    recordOptions.insert("scale-factor", 1.2);
    ui->recordButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_circle, recordOptions));
    ui->recordButton->setIconSize(QSize(32, 32));
    ui->recordButton->setText("");

    QVariantMap playOptions;
    playOptions.insert("color", QColor(52, 152, 219));  // Blue
    playOptions.insert("scale-factor", 1.2);
    ui->playButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_play, playOptions));
    ui->playButton->setIconSize(QSize(32, 32));
    ui->playButton->setText("");

    // Set initial tooltips
    ui->recordButton->setToolTip("Start Recording (Ctrl+B)");
    ui->playButton->setToolTip("No Path Available");

    // Disable play button initially
    ui->playButton->setEnabled(false);

    // Apply modern stylesheet for compact square window
    QString styleSheet = R"(
        /* Compact Window Background */
        QWidget#CompactWindow {
            background-color: #F5F7FA;
        }

        /* Record Button - Red/Green Theme with circular style */
        QPushButton#recordButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #E74C3C, stop:1 #C0392B);
            color: white;
            border: none;
            border-radius: 30px;
            font-weight: bold;
        }

        QPushButton#recordButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #EC7063, stop:1 #E74C3C);
        }

        QPushButton#recordButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #C0392B, stop:1 #A93226);
        }

        QPushButton#recordButton:checked {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #E67E22, stop:1 #D35400);
        }

        QPushButton#recordButton:checked:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #F39C12, stop:1 #E67E22);
        }

        /* Play Button - Blue Theme with circular style */
        QPushButton#playButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #3498DB, stop:1 #2980B9);
            color: white;
            border: none;
            border-radius: 30px;
            font-weight: bold;
        }

        QPushButton#playButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #5DADE2, stop:1 #3498DB);
        }

        QPushButton#playButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #2980B9, stop:1 #21618C);
        }

        QPushButton#playButton:disabled {
            background: #BDC3C7;
            color: #7F8C8D;
        }

        /* Status Label - Compact style */
        QLabel#statusLabel {
            color: #2C3E50;
            font-size: 9px;
            font-weight: 500;
            background-color: white;
            border-radius: 4px;
            padding: 4px;
        }
    )";

    this->setStyleSheet(styleSheet);
}

CompactWindow::~CompactWindow()
{
    delete ui;
}

// 设置共享组件引用
void CompactWindow::setComponents(MouseRecorder *recorder,
                                   MousePlayer *player,
                                   PathManager *pathManager,
                                   HotkeyManager *hotkeyManager)
{
    m_recorder = recorder;
    m_player = player;
    m_pathManager = pathManager;
    m_hotkeyManager = hotkeyManager;

    // Connect signals after components are set
    connectSignals();

    // Update button states
    updateRecordButton();
    updatePlayButton();
}

// 设置播放速度
void CompactWindow::setPlaybackSpeed(double speed)
{
    m_playbackSpeed = speed;
}

// 设置重复次数
void CompactWindow::setRepeatCount(int count)
{
    m_repeatCount = count;
}

// 设置录制热键
void CompactWindow::setRecordingHotkey(const QString& hotkey)
{
    m_recordingHotkey = hotkey;
}

// 显示事件处理：重新注册热键到小窗口
void CompactWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // Re-register hotkeys for this window when shown
    if (m_hotkeyManager) {
        // Temporarily set this window as the main window for hotkey registration
        m_hotkeyManager->setMainWindow(this);

        // Re-register recording hotkey
        m_hotkeyManager->registerRecordingHotkey(m_recordingHotkey);
    }
}

// 关闭事件处理：通知父窗口恢复显示
void CompactWindow::closeEvent(QCloseEvent *event)
{
    // Disconnect compact window's hotkey signals
    if (m_hotkeyManager) {
        disconnect(m_hotkeyManager, &HotkeyManager::recordingHotkeyPressed, this, &CompactWindow::onRecordingHotkeyPressed);
        disconnect(m_hotkeyManager, &HotkeyManager::stopPlaybackHotkeyPressed, this, &CompactWindow::onStopPlaybackHotkeyPressed);
    }

    // Show parent window when closing
    if (parentWidget()) {
        parentWidget()->show();
    }
    QWidget::closeEvent(event);
}

// 连接信号和槽
void CompactWindow::connectSignals()
{
    if (!m_recorder || !m_player || !m_pathManager || !m_hotkeyManager) {
        return;
    }

    // Recording signals
    connect(ui->recordButton, &QPushButton::clicked, this, &CompactWindow::onRecordButtonClicked);
    connect(m_recorder, &MouseRecorder::recordingStarted, this, &CompactWindow::onRecordingStarted);
    connect(m_recorder, &MouseRecorder::recordingStopped, this, &CompactWindow::onRecordingStopped);
    connect(m_recorder, &MouseRecorder::pointRecorded, this, &CompactWindow::onPointRecorded);

    // Playback signals
    connect(ui->playButton, &QPushButton::clicked, this, &CompactWindow::onPlayButtonClicked);
    connect(m_player, &MousePlayer::playbackStarted, this, &CompactWindow::onPlaybackStarted);
    connect(m_player, &MousePlayer::playbackFinished, this, &CompactWindow::onPlaybackFinished);
    connect(m_player, &MousePlayer::playbackStopped, this, &CompactWindow::onPlaybackStopped);

    // Path management signals
    connect(m_pathManager, &PathManager::pathSaved, this, &CompactWindow::onPathSaved);

    // Hotkey signals
    connect(m_hotkeyManager, &HotkeyManager::recordingHotkeyPressed, this, &CompactWindow::onRecordingHotkeyPressed);
    connect(m_hotkeyManager, &HotkeyManager::stopPlaybackHotkeyPressed, this, &CompactWindow::onStopPlaybackHotkeyPressed);
}

// 录制按钮点击处理
void CompactWindow::onRecordButtonClicked()
{
    if (!m_recorder) return;

    if (m_recorder->isRecording()) {
        m_recorder->stopRecording();
    } else {
        m_recorder->startRecording();
    }
}

// 录制开始事件处理
void CompactWindow::onRecordingStarted()
{
    m_recordedPointsCount = 0;
    ui->recordButton->setChecked(true);
    updateRecordButton();
    updateStatus("Recording...");
    ui->playButton->setEnabled(false);
}

// 录制停止事件处理
void CompactWindow::onRecordingStopped()
{
    ui->recordButton->setChecked(false);
    updateRecordButton();

    // Save the recorded path
    QList<MousePoint> recordedPath = m_recorder->getRecordedPath();

    if (!recordedPath.isEmpty()) {
        if (m_pathManager->savePath(recordedPath)) {
            updateStatus(QString("Saved: %1 points").arg(recordedPath.size()));
        } else {
            updateStatus("Save failed!");
        }
    } else {
        updateStatus("No movement recorded");
    }
}

// 录制到新点事件处理
void CompactWindow::onPointRecorded(const MousePoint& point)
{
    Q_UNUSED(point)
    m_recordedPointsCount++;

    // Update status every 100 points to avoid too frequent updates
    if (m_recordedPointsCount % 100 == 0) {
        updateStatus(QString("Recording... (%1)").arg(m_recordedPointsCount));
    }
}

// 播放按钮点击处理
void CompactWindow::onPlayButtonClicked()
{
    if (!m_player || !m_pathManager || m_lastSavedPath.isEmpty()) {
        updateStatus("No path to play");
        return;
    }

    QList<MousePoint> path = m_pathManager->loadPath(m_lastSavedPath);
    if (path.isEmpty()) {
        updateStatus("Failed to load path");
        return;
    }

    // 初始化重复播放状态
    m_currentPlaybackPath = path;
    m_totalRepeats = m_repeatCount;
    m_remainingRepeats = m_totalRepeats - 1;

    m_player->setPlaybackSpeed(m_playbackSpeed);
    m_player->playPath(path);
}

// 停止按钮点击处理
void CompactWindow::onStopButtonClicked()
{
    if (m_player) {
        m_player->stopPlaying();
    }
}

// 播放开始事件处理
void CompactWindow::onPlaybackStarted()
{
    ui->playButton->setEnabled(false);
    ui->recordButton->setEnabled(false);
    updateStatus("Playing... (ESC to stop)");
}

// 播放完成事件处理
void CompactWindow::onPlaybackFinished()
{
    // 检查是否还需要重复播放
    if (m_remainingRepeats > 0) {
        m_remainingRepeats--;

        // 更新状态显示当前进度
        int currentRepeat = m_totalRepeats - m_remainingRepeats;
        updateStatus(QString("Playing %1/%2...").arg(currentRepeat).arg(m_totalRepeats));

        // 继续播放
        m_player->setPlaybackSpeed(m_playbackSpeed);
        m_player->playPath(m_currentPlaybackPath);
    } else {
        // 所有重复播放完成
        ui->playButton->setEnabled(true);
        ui->recordButton->setEnabled(true);

        if (m_totalRepeats > 1) {
            updateStatus(QString("Completed (%1x)").arg(m_totalRepeats));
        } else {
            updateStatus("Playback completed");
        }

        // 清空播放路径
        m_currentPlaybackPath.clear();
    }
}

// 播放停止事件处理
void CompactWindow::onPlaybackStopped()
{
    ui->playButton->setEnabled(true);
    ui->recordButton->setEnabled(true);

    if (m_totalRepeats > 1) {
        int completedRepeats = m_totalRepeats - m_remainingRepeats;
        updateStatus(QString("Stopped (%1/%2)").arg(completedRepeats).arg(m_totalRepeats));
    } else {
        updateStatus("Playback stopped");
    }

    // 清空播放路径
    m_currentPlaybackPath.clear();
    m_remainingRepeats = 0;
}

// 路径保存事件处理
void CompactWindow::onPathSaved(const QString& filename)
{
    m_lastSavedPath = filename;
    updatePlayButton();
}

// 全局录制热键按下处理
void CompactWindow::onRecordingHotkeyPressed()
{
    // Only respond to hotkey if we're not currently playing back
    if (m_player && !m_player->isPlaying()) {
        onRecordButtonClicked();
    }
}

// 全局停止播放热键按下处理
void CompactWindow::onStopPlaybackHotkeyPressed()
{
    onStopButtonClicked();
}

// 更新录制按钮图标
void CompactWindow::updateRecordButton()
{
    QVariantMap options;
    options.insert("scale-factor", 1.2);

    if (m_recorder && m_recorder->isRecording()) {
        // Orange/Yellow when recording
        options.insert("color", QColor(230, 126, 34));
        ui->recordButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_stop, options));
        ui->recordButton->setToolTip("Stop Recording");
    } else {
        // Red when ready to record
        options.insert("color", QColor(231, 76, 60));
        ui->recordButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_circle, options));
        ui->recordButton->setToolTip("Start Recording (Ctrl+B)");
    }
}

// 更新播放按钮状态
void CompactWindow::updatePlayButton()
{
    if (!m_lastSavedPath.isEmpty()) {
        ui->playButton->setEnabled(true);
        ui->playButton->setToolTip("Play Last Path");
    } else {
        ui->playButton->setEnabled(false);
        ui->playButton->setToolTip("No Path Available");
    }
}

// 更新状态标签
void CompactWindow::updateStatus(const QString& status)
{
    ui->statusLabel->setText(status);
}
