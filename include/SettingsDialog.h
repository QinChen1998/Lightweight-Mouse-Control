#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QKeySequenceEdit>
#include <QSettings>
#include <QTimer>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // Settings access
    QString getRecordingHotkey() const;
    int getRecordingInterval() const;
    double getDefaultPlaybackSpeed() const;
    bool getMinimizeToTray() const;

    void setRecordingHotkey(const QString& hotkey);
    void setRecordingInterval(int interval);
    void setDefaultPlaybackSpeed(double speed);
    void setMinimizeToTray(bool minimize);

signals:
    void settingsChanged();

private slots:
    void onOkClicked();
    void onCancelClicked();
    void onRestoreDefaultsClicked();
    void onIntervalValueChanged(int value);
    void onIntervalEditingFinished();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void restoreDefaults();
    void showIntervalWarning(int interval);
    void updateMaxDurationDisplay();

    // UI components
    QKeySequenceEdit *m_hotkeyEdit;
    QSpinBox *m_intervalSpinBox;
    QDoubleSpinBox *m_speedSpinBox;
    QCheckBox *m_minimizeCheckBox;
    QLabel *m_maxDurationLabel;

    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    QPushButton *m_restoreDefaultsButton;

    // Settings storage
    QSettings *m_settings;

    // Timer for delayed validation
    QTimer *m_validationTimer;
};

#endif // SETTINGSDIALOG_H