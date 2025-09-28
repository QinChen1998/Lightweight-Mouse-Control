#include "settingsdialog.h"
#include <QKeySequence>
#include <QMessageBox>
#include <QTimer>

// 设置对话框构造函数：初始化设置存储和验证定时器
SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_settings(new QSettings("LightweightMouseControl", "Settings", this))
    , m_validationTimer(new QTimer(this))
{
    m_validationTimer->setSingleShot(true);
    m_validationTimer->setInterval(500); // 500ms delay
    connect(m_validationTimer, &QTimer::timeout, this, &SettingsDialog::onIntervalEditingFinished);

    setupUI();
    loadSettings();
}

// 设置UI界面：创建所有设置控件和布局
void SettingsDialog::setupUI()
{
    setWindowTitle("Settings");
    setModal(true);
    setFixedSize(400, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Hotkey settings group
    QGroupBox *hotkeyGroup = new QGroupBox("Hotkey Settings");
    QFormLayout *hotkeyLayout = new QFormLayout(hotkeyGroup);

    m_hotkeyEdit = new QKeySequenceEdit();
    m_hotkeyEdit->setToolTip("Press the key combination you want to use for recording");
    hotkeyLayout->addRow("Recording Hotkey:", m_hotkeyEdit);

    // Recording settings group
    QGroupBox *recordingGroup = new QGroupBox("Recording Settings");
    QFormLayout *recordingLayout = new QFormLayout(recordingGroup);

    m_intervalSpinBox = new QSpinBox();
    m_intervalSpinBox->setRange(1, 1000);
    m_intervalSpinBox->setSuffix(" ms");
    m_intervalSpinBox->setToolTip("Interval between recorded mouse positions\n1-9ms: Ultra-precise (may consume high CPU)\n10-49ms: High precision\n50-100ms: Balanced (recommended)\n100+ms: Low precision");
    recordingLayout->addRow("Recording Interval:", m_intervalSpinBox);

    // Playback settings group
    QGroupBox *playbackGroup = new QGroupBox("Playback Settings");
    QFormLayout *playbackLayout = new QFormLayout(playbackGroup);

    m_speedSpinBox = new QDoubleSpinBox();
    m_speedSpinBox->setRange(0.1, 5.0);
    m_speedSpinBox->setSingleStep(0.1);
    m_speedSpinBox->setToolTip("Default playback speed");
    playbackLayout->addRow("Default Speed:", m_speedSpinBox);

    // Application settings group
    QGroupBox *appGroup = new QGroupBox("Application Settings");
    QFormLayout *appLayout = new QFormLayout(appGroup);

    m_minimizeCheckBox = new QCheckBox();
    m_minimizeCheckBox->setToolTip("Minimize to system tray when closed");
    appLayout->addRow("Minimize to Tray:", m_minimizeCheckBox);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_restoreDefaultsButton = new QPushButton("Restore Defaults");
    m_cancelButton = new QPushButton("Cancel");
    m_okButton = new QPushButton("OK");
    m_okButton->setDefault(true);

    buttonLayout->addWidget(m_restoreDefaultsButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);

    // Main layout
    mainLayout->addWidget(hotkeyGroup);
    mainLayout->addWidget(recordingGroup);
    mainLayout->addWidget(playbackGroup);
    mainLayout->addWidget(appGroup);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(m_okButton, &QPushButton::clicked, this, &SettingsDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);
    connect(m_restoreDefaultsButton, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaultsClicked);
    connect(m_intervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onIntervalValueChanged);
}

// 加载设置：从注册表加载并应用到UI控件
void SettingsDialog::loadSettings()
{
    // Load settings with defaults
    QString hotkey = m_settings->value("hotkey", "Ctrl+B").toString();
    int interval = m_settings->value("recordingInterval", 50).toInt();
    double speed = m_settings->value("defaultSpeed", 1.0).toDouble();
    bool minimize = m_settings->value("minimizeToTray", false).toBool();

    // Set UI values
    m_hotkeyEdit->setKeySequence(QKeySequence(hotkey));
    m_intervalSpinBox->setValue(interval);
    m_speedSpinBox->setValue(speed);
    m_minimizeCheckBox->setChecked(minimize);
}

// 保存设置：将UI控件的值保存到注册表
void SettingsDialog::saveSettings()
{
    m_settings->setValue("hotkey", m_hotkeyEdit->keySequence().toString());
    m_settings->setValue("recordingInterval", m_intervalSpinBox->value());
    m_settings->setValue("defaultSpeed", m_speedSpinBox->value());
    m_settings->setValue("minimizeToTray", m_minimizeCheckBox->isChecked());
    m_settings->sync();
}

// 恢复默认设置：将所有设置重置为默认值
void SettingsDialog::restoreDefaults()
{
    m_hotkeyEdit->setKeySequence(QKeySequence("Ctrl+B"));
    m_intervalSpinBox->setValue(50);
    m_speedSpinBox->setValue(1.0);
    m_minimizeCheckBox->setChecked(false);
}

// 获取录制热键设置
QString SettingsDialog::getRecordingHotkey() const
{
    return m_settings->value("hotkey", "Ctrl+B").toString();
}

// 获取录制间隔设置
int SettingsDialog::getRecordingInterval() const
{
    return m_settings->value("recordingInterval", 50).toInt();
}

// 获取默认播放速度设置
double SettingsDialog::getDefaultPlaybackSpeed() const
{
    return m_settings->value("defaultSpeed", 1.0).toDouble();
}

// 获取是否最小化到系统托盘设置
bool SettingsDialog::getMinimizeToTray() const
{
    return m_settings->value("minimizeToTray", false).toBool();
}

// 设置录制热键
void SettingsDialog::setRecordingHotkey(const QString& hotkey)
{
    m_settings->setValue("hotkey", hotkey);
    m_hotkeyEdit->setKeySequence(QKeySequence(hotkey));
}

// 设置录制间隔
void SettingsDialog::setRecordingInterval(int interval)
{
    m_settings->setValue("recordingInterval", interval);
    m_intervalSpinBox->setValue(interval);
}

// 设置默认播放速度
void SettingsDialog::setDefaultPlaybackSpeed(double speed)
{
    m_settings->setValue("defaultSpeed", speed);
    m_speedSpinBox->setValue(speed);
}

// 设置是否最小化到系统托盘
void SettingsDialog::setMinimizeToTray(bool minimize)
{
    m_settings->setValue("minimizeToTray", minimize);
    m_minimizeCheckBox->setChecked(minimize);
}

// 确定按钮点击处理：保存设置并关闭对话框
void SettingsDialog::onOkClicked()
{
    saveSettings();
    emit settingsChanged();
    accept();
}

// 取消按钮点击处理：不保存直接关闭
void SettingsDialog::onCancelClicked()
{
    reject();
}

// 恢复默认设置按钮点击处理：显示确认对话框
void SettingsDialog::onRestoreDefaultsClicked()
{
    int ret = QMessageBox::question(this, "Restore Defaults",
                                   "Are you sure you want to restore all settings to their default values?",
                                   QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        restoreDefaults();
    }
}

void SettingsDialog::onIntervalValueChanged(int value)
{
    // Restart the timer each time the value changes
    // This delays the validation until user stops changing the value
    m_validationTimer->stop();
    m_validationTimer->start();
}

// 间隔编辑完成处理：检查是否需要显示警告
void SettingsDialog::onIntervalEditingFinished()
{
    int value = m_intervalSpinBox->value();
    if (value <= 5) {
        showIntervalWarning(value);
    }
}

// 显示间隔警告：对低间隔设置显示性能警告
void SettingsDialog::showIntervalWarning(int interval)
{
    QString message;
    if (interval <= 1) {
        message = QString("Warning: 1ms interval is extremely aggressive and may cause:\n"
                         "• Very high CPU usage\n"
                         "• System performance issues\n"
                         "• Excessive memory consumption\n"
                         "• Large file sizes\n\n"
                         "Consider using 10ms or higher for normal use.");
    } else if (interval <= 5) {
        message = QString("Caution: %1ms interval is very aggressive and may cause:\n"
                         "• High CPU usage during recording\n"
                         "• Large path files\n"
                         "• Potential system lag\n\n"
                         "Recommended: 10ms or higher for general use.").arg(interval);
    }

    if (!message.isEmpty()) {
        QMessageBox::warning(this, "Recording Interval Warning", message);
    }
}
