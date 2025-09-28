#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QObject>
#include <QAbstractNativeEventFilter>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class HotkeyManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit HotkeyManager(QObject *parent = nullptr);
    void setMainWindow(QWidget* mainWindow);
    ~HotkeyManager();

    // Hotkey management
    bool registerHotkey(int id, int modifiers, int virtualKey);
    bool unregisterHotkey(int id);
    void unregisterAllHotkeys();

    // Default hotkeys
    bool registerDefaultRecordingHotkey(); // Ctrl+B
    void unregisterDefaultRecordingHotkey();
    bool registerStopPlaybackHotkey(); // ESC
    void unregisterStopPlaybackHotkey();

    // Custom hotkey registration
    bool registerRecordingHotkey(const QString& keySequence);
    void unregisterRecordingHotkey();

    // Native event filter
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void recordingHotkeyPressed();
    void stopPlaybackHotkeyPressed();
    void hotkeyPressed(int id);

private:
    void initializeHotkeySystem();
    bool parseKeySequence(const QString& keySequence, int& modifiers, int& virtualKey);

#ifdef Q_OS_WIN
    QList<int> m_registeredHotkeys;
    HWND m_registrationHwnd; // Store the HWND used for registration
    QWidget* m_mainWindow; // Store the main window pointer
    static const int RECORDING_HOTKEY_ID = 1;
    static const int STOP_PLAYBACK_HOTKEY_ID = 2;
#endif
};

#endif // HOTKEYMANAGER_H