#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QInputDialog>
#include <QShowEvent>
#include <QThread>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 主窗口构造函数：初始化所有核心组件和UI
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_recorder(new MouseRecorder(this))     // 鼠标录制器
    , m_player(new MousePlayer(this))         // 鼠标播放器
    , m_pathManager(new PathManager(this))    // 路径文件管理器
    , m_hotkeyManager(new HotkeyManager(this)) // 全局热键管理器
    , m_settingsDialog(new SettingsDialog(this)) // 设置对话框
    , m_recordedPointsCount(0)
    , m_hotkeysRegistered(false)
{
    ui->setupUi(this);
    setupUI();
    connectSignals();
    loadSettings();
    updatePathList();
    updateUI();

    // Set the main window for hotkey manager
    m_hotkeyManager->setMainWindow(this);

    // Apply settings to update UI elements including button text
    applySettings();

    // Hotkey registration will be done in showEvent when window is fully visible
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 设置UI初始状态
void MainWindow::setupUI()
{
    // Set initial UI state
    ui->recordButton->setCheckable(true);
    ui->speedSpinBox->setValue(1.0);

    // Set status bar message
    statusBar()->showMessage("Ready");
}

// 连接所有信号和槽：建立组件间的通信机制
void MainWindow::connectSignals()
{
    // Recording signals
    connect(ui->recordButton, &QPushButton::clicked, this, &MainWindow::onRecordButtonClicked);
    connect(m_recorder, &MouseRecorder::recordingStarted, this, &MainWindow::onRecordingStarted);
    connect(m_recorder, &MouseRecorder::recordingStopped, this, &MainWindow::onRecordingStopped);
    connect(m_recorder, &MouseRecorder::pointRecorded, this, &MainWindow::onPointRecorded);

    // Playback signals
    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::onPlayButtonClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(ui->speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onSpeedChanged);
    connect(m_player, &MousePlayer::playbackStarted, this, &MainWindow::onPlaybackStarted);
    connect(m_player, &MousePlayer::playbackFinished, this, &MainWindow::onPlaybackFinished);
    connect(m_player, &MousePlayer::playbackStopped, this, &MainWindow::onPlaybackStopped);

    // Path management signals
    connect(ui->pathListWidget, &QListWidget::currentItemChanged, this, &MainWindow::onPathSelectionChanged);
    connect(ui->pathListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::onPathSelectionChanged);
    connect(ui->deletePathButton, &QPushButton::clicked, this, &MainWindow::onDeletePathClicked);
    connect(ui->batchDeleteButton, &QPushButton::clicked, this, &MainWindow::onBatchDeleteClicked);
    connect(ui->selectAllButton, &QPushButton::clicked, this, &MainWindow::onSelectAllClicked);
    connect(ui->refreshPathsButton, &QPushButton::clicked, this, &MainWindow::onRefreshPathsClicked);
    connect(ui->renamePathButton, &QPushButton::clicked, this, &MainWindow::onRenamePathClicked);
    connect(m_pathManager, &PathManager::pathSaved, this, &MainWindow::onPathSaved);
    connect(m_pathManager, &PathManager::pathDeleted, this, &MainWindow::onPathDeleted);
    connect(m_pathManager, &PathManager::pathsDeleted, this, &MainWindow::onPathsDeleted);
    connect(m_pathManager, &PathManager::pathRenamed, this, &MainWindow::onPathRenamed);

    // Menu actions
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onActionExit);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onActionAbout);
    connect(ui->actionDiagnostics, &QAction::triggered, this, &MainWindow::onActionDiagnostics);

    // Settings
    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    // Hotkey signals
    connect(m_hotkeyManager, &HotkeyManager::recordingHotkeyPressed, this, &MainWindow::onRecordingHotkeyPressed);

    // Settings signals
    connect(m_settingsDialog, &SettingsDialog::settingsChanged, this, &MainWindow::onSettingsChanged);
}

// 录制按钮点击处理：切换录制状态
void MainWindow::onRecordButtonClicked()
{
    if (m_recorder->isRecording()) {
        m_recorder->stopRecording();
    } else {
        m_recorder->startRecording();
    }
}

// 录制开始事件处理：更新UI状态和计数器
void MainWindow::onRecordingStarted()
{
    m_recordedPointsCount = 0;
    updateRecordButtonText();
    ui->recordButton->setChecked(true);
    updateRecordingStatus("Recording...");
    statusBar()->showMessage("Recording mouse movements...");
}

// 录制停止事件处理：保存录制的路径数据
void MainWindow::onRecordingStopped()
{
    updateRecordButtonText();
    ui->recordButton->setChecked(false);

    // Save the recorded path
    QList<MousePoint> recordedPath = m_recorder->getRecordedPath();
    qDebug() << "Recording stopped. Total points captured:" << recordedPath.size();

    if (!recordedPath.isEmpty()) {
        if (m_pathManager->savePath(recordedPath)) {
            updateRecordingStatus(QString("Recorded %1 points. Path saved.").arg(recordedPath.size()));
            statusBar()->showMessage("Recording saved successfully", 3000);
        } else {
            updateRecordingStatus("Recording failed to save!");
            statusBar()->showMessage("Failed to save recording", 3000);
        }
    } else {
        qDebug() << "No points recorded - check timer and recording logic";
        updateRecordingStatus("No movement recorded.");
        statusBar()->showMessage("No movement to save", 3000);
    }
}

// 录制到新点事件处理：更新点数计数显示
void MainWindow::onPointRecorded(const MousePoint& point)
{
    Q_UNUSED(point)
    m_recordedPointsCount++;
    updateRecordingStatus(QString("Recording... (%1 points)").arg(m_recordedPointsCount));
}

// 播放按钮点击处理：加载并播放选中的鼠标路径
void MainWindow::onPlayButtonClicked()
{
    if (m_currentSelectedPath.isEmpty()) {
        QMessageBox::warning(this, "No Path Selected", "Please select a path to play.");
        return;
    }

    QList<MousePoint> path = m_pathManager->loadPath(m_currentSelectedPath);
    if (path.isEmpty()) {
        QString errorMsg = m_pathManager->getLastError();
        if (errorMsg.isEmpty()) {
            errorMsg = "Unknown error occurred while loading the path.";
        }
        QMessageBox::warning(this, "Load Error", QString("Failed to load the selected path:\n\n%1").arg(errorMsg));
        return;
    }

    m_player->setPlaybackSpeed(ui->speedSpinBox->value());
    m_player->playPath(path);
}

// 停止按钮点击处理：停止当前播放
void MainWindow::onStopButtonClicked()
{
    m_player->stopPlaying();
}

// 播放开始事件处理：禁用播放和录制按钮
void MainWindow::onPlaybackStarted()
{
    ui->playButton->setEnabled(false);
    ui->stopButton->setEnabled(true);
    ui->recordButton->setEnabled(false);
    statusBar()->showMessage("Playing back mouse path...");
}

// 播放完成事件处理：恢复按钮状态
void MainWindow::onPlaybackFinished()
{
    ui->playButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->recordButton->setEnabled(true);
    statusBar()->showMessage("Playback completed", 3000);
}

// 播放停止事件处理：恢复按钮状态
void MainWindow::onPlaybackStopped()
{
    ui->playButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->recordButton->setEnabled(true);
    statusBar()->showMessage("Playback stopped", 3000);
}

// 播放速度改变处理：更新播放器的速度设置
void MainWindow::onSpeedChanged(double speed)
{
    m_player->setPlaybackSpeed(speed);
}

// 路径选择改变处理：更新UI状态和路径详情
void MainWindow::onPathSelectionChanged()
{
    QListWidgetItem* currentItem = ui->pathListWidget->currentItem();
    QList<QListWidgetItem*> selectedItems = ui->pathListWidget->selectedItems();

    if (currentItem) {
        m_currentSelectedPath = currentItem->data(Qt::UserRole).toString();
        ui->deletePathButton->setEnabled(true);
        ui->renamePathButton->setEnabled(true);
        ui->playButton->setEnabled(true);
        updatePathDetails();
    } else {
        m_currentSelectedPath.clear();
        ui->deletePathButton->setEnabled(false);
        ui->renamePathButton->setEnabled(false);
        ui->playButton->setEnabled(false);
        ui->pathDetailsTextEdit->clear();
    }

    // Enable batch delete button if multiple items are selected
    ui->batchDeleteButton->setEnabled(selectedItems.size() > 0);
}

// 删除路径按钮点击处理：删除当前选中的路径
void MainWindow::onDeletePathClicked()
{
    if (m_currentSelectedPath.isEmpty()) {
        return;
    }

    int ret = QMessageBox::question(this, "Delete Path",
                                   "Are you sure you want to delete this path?",
                                   QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (m_pathManager->deletePath(m_currentSelectedPath)) {
            statusBar()->showMessage("Path deleted", 3000);
        } else {
            QMessageBox::warning(this, "Delete Error", "Failed to delete the path.");
        }
    }
}

// 刷新路径按钮点击处理：重新加载路径列表
void MainWindow::onRefreshPathsClicked()
{
    updatePathList();
    statusBar()->showMessage("Path list refreshed", 2000);
}

// 重命名路径按钮点击处理：显示输入框并重命名文件
void MainWindow::onRenamePathClicked()
{
    if (m_currentSelectedPath.isEmpty()) {
        return;
    }

    // Get current display name (without .mpath extension)
    QString currentBaseName = QFileInfo(m_currentSelectedPath).baseName();
    QString currentDisplayName = m_pathManager->getDisplayName(m_currentSelectedPath);

    // Show input dialog
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Path",
                                           QString("Enter new name for '%1':").arg(currentDisplayName),
                                           QLineEdit::Normal, currentBaseName, &ok);

    if (ok && !newName.isEmpty()) {
        // Remove any .mpath extension from user input (will be added automatically)
        if (newName.endsWith(".mpath")) {
            newName.chop(6);
        }

        if (m_pathManager->renamePath(m_currentSelectedPath, newName)) {
            statusBar()->showMessage("Path renamed successfully", 3000);
        } else {
            QString errorMsg = m_pathManager->getLastError();
            if (errorMsg.isEmpty()) {
                errorMsg = "Unknown error occurred while renaming the path.";
            }
            QMessageBox::warning(this, "Rename Error", QString("Failed to rename the path:\n\n%1").arg(errorMsg));
        }
    }
}

void MainWindow::onPathSaved(const QString& filename)
{
    updatePathList();

    // Select the newly saved path
    for (int i = 0; i < ui->pathListWidget->count(); ++i) {
        QListWidgetItem* item = ui->pathListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == filename) {
            ui->pathListWidget->setCurrentItem(item);
            break;
        }
    }
}

void MainWindow::onPathDeleted(const QString& filename)
{
    Q_UNUSED(filename)
    updatePathList();
}

// 批量删除按钮点击处理：删除多个选中的路径
void MainWindow::onBatchDeleteClicked()
{
    QList<QListWidgetItem*> selectedItems = ui->pathListWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    int ret = QMessageBox::question(this, "Delete Paths",
                                   QString("Are you sure you want to delete %1 selected path(s)?").arg(selectedItems.size()),
                                   QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        QStringList filesToDelete;
        for (QListWidgetItem* item : selectedItems) {
            filesToDelete.append(item->data(Qt::UserRole).toString());
        }

        if (m_pathManager->deletePaths(filesToDelete)) {
            statusBar()->showMessage(QString("Successfully deleted %1 path(s)").arg(filesToDelete.size()), 3000);
        } else {
            QMessageBox::warning(this, "Delete Error", "Failed to delete some paths.");
        }
    }
}

// 全选按钮点击处理：选中所有路径
void MainWindow::onSelectAllClicked()
{
    ui->pathListWidget->selectAll();
}

void MainWindow::onPathsDeleted(const QStringList& filenames)
{
    Q_UNUSED(filenames)
    updatePathList();
}

void MainWindow::onPathRenamed(const QString& oldFilename, const QString& newFilename)
{
    Q_UNUSED(oldFilename);
    updatePathList();

    // Select the renamed path
    for (int i = 0; i < ui->pathListWidget->count(); ++i) {
        QListWidgetItem* item = ui->pathListWidget->item(i);
        if (item->data(Qt::UserRole).toString() == newFilename) {
            ui->pathListWidget->setCurrentItem(item);
            break;
        }
    }
}

// 退出菜单动作处理
void MainWindow::onActionExit()
{
    close();
}

// 关于菜单动作处理：显示应用程序信息
void MainWindow::onActionAbout()
{
    QMessageBox::about(this, "About Lightweight Mouse Control",
                      "Lightweight Mouse Control v1.0\n\n"
                      "A simple tool for recording and replaying mouse movements.\n\n"
                      "Features:\n"
                      "• Record mouse movements with Ctrl+B\n"
                      "• Save recordings with date-based filenames\n"
                      "• Replay saved paths at different speeds\n"
                      "• Manage and organize your recordings\n\n"
                      "Built with Qt and C++");
}

// 诊断菜单动作处理：显示鼠标控制诊断信息
void MainWindow::onActionDiagnostics()
{
    QString diagnostics;
    diagnostics += "=== Mouse Control Diagnostics ===\n\n";

    // Current mouse position
    QPoint currentPos = QCursor::pos();
    diagnostics += QString("Qt Mouse Position: (%1, %2) [LEGACY - NOT USED]\n").arg(currentPos.x()).arg(currentPos.y());

#ifdef Q_OS_WIN
    // Windows API position
    POINT winPos;
    if (GetCursorPos(&winPos)) {
        diagnostics += QString("Windows API Position: (%1, %2) [NOW USED FOR RECORDING]\n").arg(winPos.x).arg(winPos.y);

        if (QPoint(winPos.x, winPos.y) != currentPos) {
            diagnostics += "️NOTE: Different coordinate systems detected (this is expected)\n";

            // Calculate scaling factor
            double scaleX = (double)winPos.x / currentPos.x();
            double scaleY = (double)winPos.y / currentPos.y();
            diagnostics += QString("Coordinate Scale Factor: X=%1, Y=%2\n").arg(scaleX, 0, 'f', 2).arg(scaleY, 0, 'f', 2);
        } else {
            diagnostics += "Qt and Windows API positions match\n";
        }
    }

    // DPI information
    HDC hdc = GetDC(NULL);
    if (hdc) {
        int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        diagnostics += QString("System DPI: %1 x %2\n").arg(dpiX).arg(dpiY);

        if (dpiX != 96 || dpiY != 96) {
            diagnostics += "WARNING: Non-standard DPI detected! This may affect accuracy.\n";
        }

        ReleaseDC(NULL, hdc);
    }

    // Screen resolution
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    diagnostics += QString("Screen Resolution: %1 x %2\n").arg(screenWidth).arg(screenHeight);

    // Virtual screen (multi-monitor)
    int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (virtualWidth != screenWidth || virtualHeight != screenHeight) {
        diagnostics += QString("Virtual Screen: %1 x %2\n").arg(virtualWidth).arg(virtualHeight);
        diagnostics += "Multi-monitor setup detected.\n";
    }
#endif

    // Current settings
    diagnostics += QString("\n=== Current Settings ===\n");
    diagnostics += QString("Recording Interval: %1ms\n").arg(m_settingsDialog->getRecordingInterval());
    diagnostics += QString("Playback Speed: %1x\n").arg(m_settingsDialog->getDefaultPlaybackSpeed());

    // Recent path information
    if (!m_currentSelectedPath.isEmpty()) {
        diagnostics += QString("\n=== Selected Path Info ===\n");
        diagnostics += QString("File: %1\n").arg(m_currentSelectedPath);

        QList<MousePoint> path = m_pathManager->loadPath(m_currentSelectedPath);
        if (!path.isEmpty()) {
            diagnostics += QString("Points: %1\n").arg(path.size());
            diagnostics += QString("First Point: (%1, %2)\n").arg(path.first().position().x()).arg(path.first().position().y());
            diagnostics += QString("Last Point: (%1, %2)\n").arg(path.last().position().x()).arg(path.last().position().y());

            qint64 duration = path.last().timestamp().toMSecsSinceEpoch() - path.first().timestamp().toMSecsSinceEpoch();
            diagnostics += QString("Duration: %1ms\n").arg(duration);
        }
    }

    diagnostics += "\n=== Troubleshooting Tips ===\n";
    diagnostics += "• Ensure same display setup when recording/replaying\n";
    diagnostics += "• Close other applications that might interfere\n";
    diagnostics += "• Try different recording intervals (1-100ms)\n";

    // Add specific DPI scaling advice
#ifdef Q_OS_WIN
    POINT winPos2;
    QPoint qtPos2 = QCursor::pos();
    if (GetCursorPos(&winPos2) && QPoint(winPos2.x, winPos2.y) != qtPos2) {
        diagnostics += "• DPI SCALING DETECTED AND HANDLED:\n";
        diagnostics += "  - Recording: Uses Windows API coordinates\n";
        diagnostics += "  - Playback: Uses Windows API coordinates\n";
        diagnostics += "  - Both systems now use the same coordinate space\n";
        diagnostics += "• Please test a new recording to verify accuracy\n";
    } else {
        diagnostics += "• No DPI scaling issues detected\n";
    }
#endif

    diagnostics += "• For best results, keep display settings unchanged between recording/playback\n";

    QMessageBox::information(this, "Mouse Control Diagnostics", diagnostics);
}

// 设置按钮点击处理：打开设置对话框
void MainWindow::onSettingsClicked()
{
    m_settingsDialog->exec();
}

// 更新UI状态：根据当前录制和播放状态调整按钮
void MainWindow::updateUI()
{
    // Update button states based on current state
    bool isRecording = m_recorder->isRecording();
    bool isPlaying = m_player->isPlaying();

    ui->recordButton->setEnabled(!isPlaying);
    ui->playButton->setEnabled(!isRecording && !isPlaying && !m_currentSelectedPath.isEmpty());
    ui->stopButton->setEnabled(isPlaying);

    if (!isRecording && !isPlaying) {
        updateRecordingStatus("Ready to record");
    }
}

// 更新路径列表：从文件系统重新加载所有保存的路径
void MainWindow::updatePathList()
{
    ui->pathListWidget->clear();

    QStringList paths = m_pathManager->getAvailablePaths();
    for (const QString& filename : paths) {
        QString displayName = m_pathManager->getDisplayName(filename);

        QListWidgetItem* item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, filename);
        item->setToolTip(filename);

        ui->pathListWidget->addItem(item);
    }

    if (paths.isEmpty()) {
        ui->pathDetailsTextEdit->setPlainText("No saved paths found.\nRecord a new path to get started!");
    }
}

// 更新路径详情：显示当前选中路径的详细信息
void MainWindow::updatePathDetails()
{
    if (m_currentSelectedPath.isEmpty()) {
        ui->pathDetailsTextEdit->clear();
        return;
    }

    QString pathInfo = m_pathManager->getPathInfo(m_currentSelectedPath);
    ui->pathDetailsTextEdit->setPlainText(pathInfo);
}

// 更新录制状态显示
void MainWindow::updateRecordingStatus(const QString& status)
{
    ui->recordingStatusLabel->setText(status);
}

// 更新录制按钮文本：根据当前状态和热键设置
void MainWindow::updateRecordButtonText()
{
    QString hotkey = m_settingsDialog->getRecordingHotkey();
    if (m_recorder->isRecording()) {
        ui->recordButton->setText(QString("Stop Recording (%1)").arg(hotkey));
    } else {
        ui->recordButton->setText(QString("Start Recording (%1)").arg(hotkey));
    }
}

// 全局录制热键按下处理：在非播放状态下切换录制
void MainWindow::onRecordingHotkeyPressed()
{
    // Only respond to hotkey if we're not currently playing back
    if (!m_player->isPlaying()) {
        onRecordButtonClicked();
    }
}

// 设置改变处理：应用新设置到所有组件
void MainWindow::onSettingsChanged()
{
    applySettings();
    statusBar()->showMessage("Settings updated", 2000);
}

// 加载设置：从设置对话框加载并应用配置
void MainWindow::loadSettings()
{
    // Apply recording interval
    int interval = m_settingsDialog->getRecordingInterval();
    m_recorder->setRecordingInterval(interval);

    // Apply default playback speed
    double speed = m_settingsDialog->getDefaultPlaybackSpeed();
    ui->speedSpinBox->setValue(speed);
    m_player->setPlaybackSpeed(speed);

    // Update interval display
    updateIntervalDisplay();
}

// 应用设置：将设置应用到所有组件并重新注册热键
void MainWindow::applySettings()
{
    loadSettings();

    // Update hotkey label and button text
    QString hotkey = m_settingsDialog->getRecordingHotkey();
    ui->hotkeyLabel->setText(QString("Global Hotkey: %1").arg(hotkey));
    updateRecordButtonText();

    // Update interval display
    updateIntervalDisplay();

    // Re-register hotkey with new combination
    if (m_hotkeysRegistered) {
        // Add a small delay to ensure unregistration is complete
        QThread::msleep(100);

        bool success = m_hotkeyManager->registerRecordingHotkey(hotkey);
        if (success) {
            statusBar()->showMessage(QString("Hotkey updated to %1").arg(hotkey), 3000);
        } else {
            statusBar()->showMessage(QString("Failed to update hotkey to %1").arg(hotkey), 5000);

            // Try to fallback to a default hotkey if the new one failed
            bool fallbackSuccess = m_hotkeyManager->registerRecordingHotkey("Ctrl+B");
            if (fallbackSuccess) {
                statusBar()->showMessage("Hotkey registration failed, reverted to Ctrl+B", 5000);
            } else {
                statusBar()->showMessage("ERROR: No hotkey registered!", 5000);
                m_hotkeysRegistered = false;
            }
        }
    }
}

// 更新录制间隔显示：根据间隔值显示不同的精度级别
void MainWindow::updateIntervalDisplay()
{
    int interval = m_settingsDialog->getRecordingInterval();
    QString displayText;

    if (interval < 10) {
        displayText = QString("Recording Interval: %1ms (Ultra-precise)").arg(interval);
    } else if (interval < 50) {
        displayText = QString("Recording Interval: %1ms (High precision)").arg(interval);
    } else if (interval <= 100) {
        displayText = QString("Recording Interval: %1ms (Balanced)").arg(interval);
    } else {
        displayText = QString("Recording Interval: %1ms (Low precision)").arg(interval);
    }

    ui->intervalLabel->setText(displayText);
}

// 窗口显示事件处理：窗口显示时注册全局热键
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // Register hotkeys when the window is fully shown
    if (!m_hotkeysRegistered) {
        registerHotkeysWhenReady();
    }
}

// 注册热键：在窗口准备好后注册全局热键
void MainWindow::registerHotkeysWhenReady()
{
    if (m_hotkeysRegistered) {
        return;
    }

    // Use the hotkey from settings
    QString configuredHotkey = m_settingsDialog->getRecordingHotkey();
    bool hotkeyRegistered = m_hotkeyManager->registerRecordingHotkey(configuredHotkey);

    if (!hotkeyRegistered) {
        QString warningMsg = QString("Warning: Failed to register global hotkey %1").arg(configuredHotkey);
        qWarning() << warningMsg;
        statusBar()->showMessage(warningMsg, 5000);
    } else {
        statusBar()->showMessage(QString("Global hotkey %1 registered successfully").arg(configuredHotkey), 3000);
        m_hotkeysRegistered = true;
    }
}
