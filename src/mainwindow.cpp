#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QInputDialog>
#include <QShowEvent>
#include <QThread>
#include <QMessageBox>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 主窗口构造函数：初始化所有核心组件和UI
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_awesome(new fa::QtAwesome(this))      // 图标库
    , m_recorder(new MouseRecorder(this))     // 鼠标录制器
    , m_player(new MousePlayer(this))         // 鼠标播放器
    , m_pathManager(new PathManager(this))    // 路径文件管理器
    , m_hotkeyManager(new HotkeyManager(this)) // 全局热键管理器
    , m_settingsDialog(new SettingsDialog(this)) // 设置对话框
    , m_compactWindow(nullptr)  // 紧凑窗口（延迟创建）
    , m_recordedPointsCount(0)
    , m_hotkeysRegistered(false)
    , m_remainingRepeats(0)
    , m_totalRepeats(1)
{
    ui->setupUi(this);

    // Initialize QtAwesome
    m_awesome->initFontAwesome();

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

    // 创建自定义SpinBox组件
    m_speedSpinBox = new CustomDoubleSpinBox(this);
    m_speedSpinBox->setRange(0.1, 5.0);
    m_speedSpinBox->setSingleStep(0.1);
    m_speedSpinBox->setValue(1.0);
    m_speedSpinBox->setSuffix("x");
    m_speedSpinBox->setDecimals(1);

    m_repeatSpinBox = new CustomSpinBox(this);
    m_repeatSpinBox->setRange(1, 1000);
    m_repeatSpinBox->setSingleStep(1);
    m_repeatSpinBox->setValue(1);
    m_repeatSpinBox->setSuffix(" times");

    // 替换UI中的SpinBox
    QLayout* playbackLayout = ui->playbackGroupBox->layout();
    if (playbackLayout) {
        // 找到原有的speedSpinBox和repeatSpinBox并替换
        QLayoutItem* item;
        for (int i = 0; i < playbackLayout->count(); ++i) {
            item = playbackLayout->itemAt(i);
            if (item && item->widget()) {
                if (item->widget()->objectName() == "speedSpinBox") {
                    QWidget* oldWidget = item->widget();
                    playbackLayout->removeWidget(oldWidget);
                    playbackLayout->addWidget(m_speedSpinBox);
                    oldWidget->hide();
                } else if (item->widget()->objectName() == "repeatSpinBox") {
                    QWidget* oldWidget = item->widget();
                    playbackLayout->removeWidget(oldWidget);
                    playbackLayout->addWidget(m_repeatSpinBox);
                    oldWidget->hide();
                }
            }
        }
    }

    // Set status bar message
    statusBar()->showMessage("Ready");

    // Set button icons using QtAwesome
    QVariantMap redOptions;
    redOptions.insert("color", QColor(231, 76, 60));
    redOptions.insert("color-active", QColor(231, 76, 60));
    ui->recordButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_circle, redOptions));

    QVariantMap blueOptions;
    blueOptions.insert("color", QColor(52, 152, 219));
    ui->playButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_play, blueOptions));

    QVariantMap orangeOptions;
    orangeOptions.insert("color", QColor(230, 126, 34));
    ui->stopButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_stop, orangeOptions));

    QVariantMap grayOptions;
    grayOptions.insert("color", QColor(127, 140, 141));
    ui->renamePathButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_pen_to_square, grayOptions));
    ui->deletePathButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_trash, QVariantMap{{"color", QColor(231, 76, 60)}}));
    ui->batchDeleteButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_trash_can, QVariantMap{{"color", QColor(231, 76, 60)}}));
    ui->selectAllButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_check_double, grayOptions));
    ui->refreshPathsButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_rotate_right, grayOptions));
    ui->settingsButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_gear, grayOptions));
    ui->compactModeButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_compress, grayOptions));

    // Apply modern stylesheet
    QString styleSheet = R"(
        /* Main Window Background */
        QMainWindow {
            background-color: #F5F7FA;
        }

        /* Central Widget */
        QWidget#centralwidget {
            background-color: #F5F7FA;
        }

        /* Modern GroupBox - Card Style */
        QGroupBox {
            background-color: white;
            border: none;
            border-radius: 8px;
            margin-top: 12px;
            padding: 15px;
            font-weight: bold;
            color: #2C3E50;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 5px 10px;
            color: #2C3E50;
            font-size: 14px;
        }

        /* Record Button - Red Theme */
        QPushButton#recordButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #E74C3C, stop:1 #C0392B);
            color: white;
            border: none;
            border-radius: 5px;
            padding: 6px 12px;
            font-weight: bold;
            font-size: 12px;
            min-height: 28px;
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
                                       stop:0 #27AE60, stop:1 #229954);
        }

        QPushButton#recordButton:checked:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #2ECC71, stop:1 #27AE60);
        }

        /* Play Button - Blue Theme */
        QPushButton#playButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #3498DB, stop:1 #2980B9);
            color: white;
            border: none;
            border-radius: 5px;
            padding: 6px 12px;
            font-weight: bold;
            font-size: 12px;
            min-height: 28px;
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

        /* Stop Button - Gray Theme */
        QPushButton#stopButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #95A5A6, stop:1 #7F8C8D);
            color: white;
            border: none;
            border-radius: 5px;
            padding: 6px 12px;
            font-weight: bold;
            font-size: 12px;
            min-height: 28px;
        }

        QPushButton#stopButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #AAB7B8, stop:1 #95A5A6);
        }

        QPushButton#stopButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                                       stop:0 #7F8C8D, stop:1 #5D6D7E);
        }

        QPushButton#stopButton:disabled {
            background: #ECF0F1;
            color: #BDC3C7;
        }

        /* Normal Buttons - White with Border */
        QPushButton {
            background-color: white;
            color: #3498DB;
            border: 2px solid #3498DB;
            border-radius: 5px;
            padding: 5px 12px;
            font-weight: 500;
            font-size: 11px;
            min-height: 26px;
        }

        QPushButton:hover {
            background-color: #EBF5FB;
            border-color: #2980B9;
        }

        QPushButton:pressed {
            background-color: #D6EAF8;
        }

        QPushButton:disabled {
            background-color: #ECF0F1;
            color: #BDC3C7;
            border-color: #BDC3C7;
        }

        /* ListWidget - Modern Style */
        QListWidget {
            background-color: white;
            border: 1px solid #E8E8E8;
            border-radius: 6px;
            padding: 5px;
            outline: none;
        }

        QListWidget::item {
            padding: 8px;
            border-radius: 4px;
            color: #2C3E50;
        }

        QListWidget::item:selected {
            background-color: #3498DB;
            color: white;
        }

        QListWidget::item:hover:!selected {
            background-color: #EBF5FB;
        }

        /* TextEdit - Modern Style */
        QTextEdit {
            background-color: white;
            border: 1px solid #E8E8E8;
            border-radius: 6px;
            padding: 8px;
            color: #2C3E50;
            selection-background-color: #3498DB;
        }


        /* Labels */
        QLabel {
            color: #2C3E50;
            font-size: 12px;
        }

        /* Status Label */
        QLabel#recordingStatusLabel {
            color: #7F8C8D;
            font-size: 13px;
            font-weight: 500;
        }

        QLabel#intervalLabel, QLabel#hotkeyLabel {
            color: #7F8C8D;
            font-size: 11px;
            padding: 5px 10px;
            background-color: white;
            border-radius: 4px;
        }

        /* StatusBar */
        QStatusBar {
            background-color: white;
            color: #7F8C8D;
            border-top: 1px solid #E8E8E8;
        }

        /* MenuBar */
        QMenuBar {
            background-color: white;
            color: #2C3E50;
            border-bottom: 1px solid #E8E8E8;
        }

        QMenuBar::item {
            padding: 5px 10px;
            background-color: transparent;
        }

        QMenuBar::item:selected {
            background-color: #EBF5FB;
            color: #3498DB;
        }

        QMenu {
            background-color: white;
            border: 1px solid #E8E8E8;
            border-radius: 4px;
        }

        QMenu::item {
            padding: 8px 25px;
            color: #2C3E50;
        }

        QMenu::item:selected {
            background-color: #3498DB;
            color: white;
        }
    )";

    this->setStyleSheet(styleSheet);
}

// 连接所有信号和槽：建立组件间的通信机制
void MainWindow::connectSignals()
{
    // Recording signals
    connect(ui->recordButton, &QPushButton::clicked, this, &MainWindow::onRecordButtonClicked);
    connect(m_recorder, &MouseRecorder::recordingStarted, this, &MainWindow::onRecordingStarted);
    connect(m_recorder, &MouseRecorder::recordingStopped, this, &MainWindow::onRecordingStopped);
    connect(m_recorder, &MouseRecorder::pointRecorded, this, &MainWindow::onPointRecorded);
    connect(m_recorder, &MouseRecorder::recordingLimitReached, this, &MainWindow::onRecordingLimitReached);

    // Playback signals
    connect(ui->playButton, &QPushButton::clicked, this, &MainWindow::onPlayButtonClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(m_speedSpinBox, &CustomDoubleSpinBox::valueChanged, this, &MainWindow::onSpeedChanged);
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
    connect(ui->actionCompactMode, &QAction::triggered, this, &MainWindow::onActionCompactMode);

    // Settings
    connect(ui->settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    // Compact mode
    connect(ui->compactModeButton, &QPushButton::clicked, this, &MainWindow::onActionCompactMode);

    // Hotkey signals
    connect(m_hotkeyManager, &HotkeyManager::recordingHotkeyPressed, this, &MainWindow::onRecordingHotkeyPressed);
    connect(m_hotkeyManager, &HotkeyManager::stopPlaybackHotkeyPressed, this, &MainWindow::onStopButtonClicked);

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

// 录制到新点事件处理：更新点数计数和剩余百分比显示
void MainWindow::onPointRecorded(const MousePoint& point)
{
    Q_UNUSED(point)
    m_recordedPointsCount++;

    // Calculate remaining percentage
    const int MAX_POINTS = 300000; // Same as MouseRecorder::MAX_RECORDING_POINTS
    double usedPercentage = (double)m_recordedPointsCount / MAX_POINTS * 100.0;
    double remainingPercentage = 100.0 - usedPercentage;

    // Format the status message
    QString statusMsg;
    if (remainingPercentage > 10.0) {
        statusMsg = QString("Recording... (%1 points, %2% remaining)")
                   .arg(m_recordedPointsCount)
                   .arg(QString::number(remainingPercentage, 'f', 1));
    } else if (remainingPercentage > 1.0) {
        statusMsg = QString("Recording... (%1 points, %2% remaining)")
                   .arg(m_recordedPointsCount)
                   .arg(QString::number(remainingPercentage, 'f', 2));
    } else {
        statusMsg = QString("Recording... (%1 points, %2% remaining - Near limit!)")
                   .arg(m_recordedPointsCount)
                   .arg(QString::number(remainingPercentage, 'f', 3));
    }

    updateRecordingStatus(statusMsg);
}

// 录制达到限制处理：显示警告信息
void MainWindow::onRecordingLimitReached()
{
    updateRecordingStatus("Recording stopped: Maximum points limit reached (300,000 points)");
    statusBar()->showMessage("Recording automatically stopped due to memory limit", 5000);

    // Show warning message box
    QMessageBox::warning(this, "Recording Limit Reached",
                        "Recording has been automatically stopped because it reached the maximum limit of 300,000 points.\n\n"
                        "This limit prevents excessive memory usage during long recordings.\n"
                        "The recorded path has been saved successfully.");
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

    // 初始化重复播放状态
    m_currentPlaybackPath = path;
    m_totalRepeats = m_repeatSpinBox->value();
    m_remainingRepeats = m_totalRepeats - 1;  // 减去即将播放的第一次

    m_player->setPlaybackSpeed(m_speedSpinBox->value());
    m_player->playPath(path);
}

// 停止按钮点击处理：停止当前播放
void MainWindow::onStopButtonClicked()
{
    m_player->stopPlaying();
}

// 播放开始事件处理：禁用播放和录制按钮，注册ESC停止热键
void MainWindow::onPlaybackStarted()
{
    ui->playButton->setEnabled(false);
    ui->stopButton->setEnabled(true);
    ui->recordButton->setEnabled(false);
    statusBar()->showMessage("Playing back mouse path... (Press ESC to stop)");

    // Register ESC hotkey to stop playback
    m_hotkeyManager->registerStopPlaybackHotkey();
}

// 播放完成事件处理：检查是否需要重复播放，或恢复按钮状态
void MainWindow::onPlaybackFinished()
{
    // 检查是否还需要重复播放
    if (m_remainingRepeats > 0) {
        m_remainingRepeats--;

        // 更新状态栏显示当前进度
        int currentRepeat = m_totalRepeats - m_remainingRepeats;
        statusBar()->showMessage(QString("Playing %1/%2...").arg(currentRepeat).arg(m_totalRepeats));

        // 继续播放
        m_player->setPlaybackSpeed(m_speedSpinBox->value());
        m_player->playPath(m_currentPlaybackPath);
    } else {
        // 所有重复播放完成，恢复UI状态
        ui->playButton->setEnabled(true);
        ui->stopButton->setEnabled(false);
        ui->recordButton->setEnabled(true);

        if (m_totalRepeats > 1) {
            statusBar()->showMessage(QString("Playback completed (%1 times)").arg(m_totalRepeats), 3000);
        } else {
            statusBar()->showMessage("Playback completed", 3000);
        }

        // Unregister ESC hotkey
        m_hotkeyManager->unregisterStopPlaybackHotkey();

        // 清空播放路径
        m_currentPlaybackPath.clear();
    }
}

// 播放停止事件处理：恢复按钮状态，取消注册ESC热键
void MainWindow::onPlaybackStopped()
{
    ui->playButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->recordButton->setEnabled(true);

    if (m_totalRepeats > 1) {
        int completedRepeats = m_totalRepeats - m_remainingRepeats;
        statusBar()->showMessage(QString("Playback stopped (%1/%2 completed)").arg(completedRepeats).arg(m_totalRepeats), 3000);
    } else {
        statusBar()->showMessage("Playback stopped", 3000);
    }

    // Unregister ESC hotkey
    m_hotkeyManager->unregisterStopPlaybackHotkey();

    // 清空播放路径和重置重复次数
    m_currentPlaybackPath.clear();
    m_remainingRepeats = 0;
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

// 紧凑模式菜单动作处理：打开小窗口并隐藏主窗口
void MainWindow::onActionCompactMode()
{
    // Create compact window if it doesn't exist
    if (!m_compactWindow) {
        m_compactWindow = new CompactWindow(this);
        m_compactWindow->setComponents(m_recorder, m_player, m_pathManager, m_hotkeyManager);
    }

    // Update compact window with current settings
    m_compactWindow->setPlaybackSpeed(m_speedSpinBox->value());
    m_compactWindow->setRepeatCount(m_repeatSpinBox->value());
    m_compactWindow->setRecordingHotkey(m_settingsDialog->getRecordingHotkey());

    // Disconnect main window's hotkey signals to avoid duplicate handling
    disconnect(m_hotkeyManager, &HotkeyManager::recordingHotkeyPressed, this, &MainWindow::onRecordingHotkeyPressed);
    disconnect(m_hotkeyManager, &HotkeyManager::stopPlaybackHotkeyPressed, this, &MainWindow::onStopButtonClicked);

    // Hide main window and show compact window
    hide();
    m_compactWindow->show();
    m_compactWindow->activateWindow();
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

// 更新录制按钮文本和图标：根据当前状态和热键设置
void MainWindow::updateRecordButtonText()
{
    QString hotkey = m_settingsDialog->getRecordingHotkey();
    QVariantMap options;

    if (m_recorder->isRecording()) {
        ui->recordButton->setText(QString("Stop Recording (%1)").arg(hotkey));
        // Orange/Yellow when recording
        options.insert("color", QColor(230, 126, 34));
        ui->recordButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_stop, options));
    } else {
        ui->recordButton->setText(QString("Start Recording (%1)").arg(hotkey));
        // Red when ready to record
        options.insert("color", QColor(231, 76, 60));
        ui->recordButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_circle, options));
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
    m_speedSpinBox->setValue(speed);
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
    } else {
        // Re-connect hotkey signals when returning from compact mode
        connect(m_hotkeyManager, &HotkeyManager::recordingHotkeyPressed, this, &MainWindow::onRecordingHotkeyPressed, Qt::UniqueConnection);
        connect(m_hotkeyManager, &HotkeyManager::stopPlaybackHotkeyPressed, this, &MainWindow::onStopButtonClicked, Qt::UniqueConnection);

        // Re-register hotkeys when returning from compact mode
        m_hotkeyManager->setMainWindow(this);
        QString hotkey = m_settingsDialog->getRecordingHotkey();
        m_hotkeyManager->registerRecordingHotkey(hotkey);
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
