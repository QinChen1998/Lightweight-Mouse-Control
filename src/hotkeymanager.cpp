#include "hotkeymanager.h"
#include <QApplication>
#include <QWidget>
#include <QDebug>
#include <QKeySequence>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 热键管理器构造函数：初始化全局热键系统
HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
#ifdef Q_OS_WIN
    , m_registrationHwnd(nullptr)
    , m_mainWindow(nullptr)
#endif
{
    initializeHotkeySystem();
}

// 设置主窗口引用：用于热键注册
void HotkeyManager::setMainWindow(QWidget* mainWindow)
{
#ifdef Q_OS_WIN
    m_mainWindow = mainWindow;
#else
    Q_UNUSED(mainWindow)
#endif
}

HotkeyManager::~HotkeyManager()
{
    unregisterAllHotkeys();
}

// 初始化热键系统：安装原生事件过滤器
void HotkeyManager::initializeHotkeySystem()
{
#ifdef Q_OS_WIN
    // Install the native event filter to capture hotkey messages
    QApplication::instance()->installNativeEventFilter(this);
#endif
}

// 注册全局热键：使用Windows API注册系统级热键
bool HotkeyManager::registerHotkey(int id, int modifiers, int virtualKey)
{
#ifdef Q_OS_WIN
    HWND hwnd = nullptr;

    // Use the stored main window if available
    if (m_mainWindow) {
        hwnd = (HWND)m_mainWindow->winId();
    } else {
        // Fallback to finding main window
        QWidget* mainWindow = QApplication::activeWindow();
        if (!mainWindow) {
            QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
            for (QWidget* widget : topLevelWidgets) {
                if (widget->isVisible() && widget->inherits("QMainWindow")) {
                    mainWindow = widget;
                    break;
                }
            }
        }

        if (mainWindow) {
            hwnd = (HWND)mainWindow->winId();
        }
    }

    if (!hwnd) {
        qWarning() << "Failed to get window handle for hotkey registration";
        return false;
    }

    bool success = RegisterHotKey(hwnd, id, modifiers, virtualKey);
    if (success) {
        m_registeredHotkeys.append(id);
        m_registrationHwnd = hwnd;
    } else {
        DWORD error = GetLastError();
        if (error == ERROR_HOTKEY_ALREADY_REGISTERED) {
            qWarning() << "Hotkey already registered by another application";
        }
    }

    return success;
#else
    Q_UNUSED(id)
    Q_UNUSED(modifiers)
    Q_UNUSED(virtualKey)
    qWarning() << "Hotkey registration not implemented for this platform";
    return false;
#endif
}

// 取消注册热键：从系统中移除指定热键
bool HotkeyManager::unregisterHotkey(int id)
{
#ifdef Q_OS_WIN
    HWND hwnd = nullptr;

    // Use the stored HWND from registration
    if (m_registrationHwnd && IsWindow(m_registrationHwnd)) {
        hwnd = m_registrationHwnd;
    } else if (m_mainWindow) {
        hwnd = (HWND)m_mainWindow->winId();
    }

    if (!hwnd) {
        return false;
    }

    bool success = UnregisterHotKey(hwnd, id);
    if (success) {
        m_registeredHotkeys.removeAll(id);
        if (m_registeredHotkeys.isEmpty()) {
            m_registrationHwnd = nullptr;
        }
    }

    return success;
#else
    Q_UNUSED(id)
    return false;
#endif
}

// 取消注册所有热键：清理所有已注册的热键
void HotkeyManager::unregisterAllHotkeys()
{
#ifdef Q_OS_WIN
    for (int id : m_registeredHotkeys) {
        unregisterHotkey(id);
    }
    m_registeredHotkeys.clear();

    // Remove the native event filter
    QApplication::instance()->removeNativeEventFilter(this);
#endif
}

// 注册默认录制热键（Ctrl+B）
bool HotkeyManager::registerDefaultRecordingHotkey()
{
#ifdef Q_OS_WIN
    // Register Ctrl+B (MOD_CONTROL + VK_B)
    return registerHotkey(RECORDING_HOTKEY_ID, MOD_CONTROL, 0x42); // 0x42 is VK_B
#else
    return false;
#endif
}

// 取消注册默认录制热键
void HotkeyManager::unregisterDefaultRecordingHotkey()
{
#ifdef Q_OS_WIN
    unregisterHotkey(RECORDING_HOTKEY_ID);
#endif
}

// 注册停止播放热键（ESC）
bool HotkeyManager::registerStopPlaybackHotkey()
{
#ifdef Q_OS_WIN
    // Register ESC key (no modifiers + VK_ESCAPE)
    return registerHotkey(STOP_PLAYBACK_HOTKEY_ID, 0, VK_ESCAPE);
#else
    return false;
#endif
}

// 取消注册停止播放热键
void HotkeyManager::unregisterStopPlaybackHotkey()
{
#ifdef Q_OS_WIN
    unregisterHotkey(STOP_PLAYBACK_HOTKEY_ID);
#endif
}

// 注册自定义录制热键：解析并注册用户指定的热键组合
bool HotkeyManager::registerRecordingHotkey(const QString& keySequence)
{
#ifdef Q_OS_WIN
    int modifiers, virtualKey;
    if (!parseKeySequence(keySequence, modifiers, virtualKey)) {
        qWarning() << "Failed to parse key sequence:" << keySequence;
        return false;
    }

    // Unregister existing recording hotkey first
    unregisterRecordingHotkey();

    // Register new hotkey
    bool success = registerHotkey(RECORDING_HOTKEY_ID, modifiers, virtualKey);

    if (!success) {
        qWarning() << "Failed to register hotkey" << keySequence << "- may be in use by another application";
    }

    return success;
#else
    Q_UNUSED(keySequence)
    return false;
#endif
}

// 取消注册录制热键
void HotkeyManager::unregisterRecordingHotkey()
{
#ifdef Q_OS_WIN
    unregisterHotkey(RECORDING_HOTKEY_ID);
#endif
}

// 解析热键序列：将Qt键位组合转换为Windows虚拟键码
bool HotkeyManager::parseKeySequence(const QString& keySequence, int& modifiers, int& virtualKey)
{
#ifdef Q_OS_WIN
    QKeySequence sequence(keySequence);
    if (sequence.isEmpty()) {
        qWarning() << "Empty key sequence provided";
        return false;
    }

    // Get the first key combination from the sequence
    int key = sequence[0];

    // Extract modifiers
    modifiers = 0;
    if (key & Qt::ControlModifier) modifiers |= MOD_CONTROL;
    if (key & Qt::AltModifier) modifiers |= MOD_ALT;
    if (key & Qt::ShiftModifier) modifiers |= MOD_SHIFT;
    if (key & Qt::MetaModifier) modifiers |= MOD_WIN;

    // Extract the actual key (remove modifiers)
    int baseKey = key & ~(Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier);

    // Convert Qt key to Windows virtual key
    virtualKey = 0;

    // Letters A-Z
    if (baseKey >= Qt::Key_A && baseKey <= Qt::Key_Z) {
        virtualKey = baseKey;
    }
    // Numbers 0-9
    else if (baseKey >= Qt::Key_0 && baseKey <= Qt::Key_9) {
        virtualKey = baseKey;
    }
    // Function keys F1-F12
    else if (baseKey >= Qt::Key_F1 && baseKey <= Qt::Key_F12) {
        virtualKey = VK_F1 + (baseKey - Qt::Key_F1);
    }
    // Special keys
    else {
        switch (baseKey) {
        case Qt::Key_Space: virtualKey = VK_SPACE; break;
        case Qt::Key_Return:
        case Qt::Key_Enter: virtualKey = VK_RETURN; break;
        case Qt::Key_Escape: virtualKey = VK_ESCAPE; break;
        case Qt::Key_Tab: virtualKey = VK_TAB; break;
        case Qt::Key_Backspace: virtualKey = VK_BACK; break;
        case Qt::Key_Delete: virtualKey = VK_DELETE; break;
        case Qt::Key_Insert: virtualKey = VK_INSERT; break;
        case Qt::Key_Home: virtualKey = VK_HOME; break;
        case Qt::Key_End: virtualKey = VK_END; break;
        case Qt::Key_PageUp: virtualKey = VK_PRIOR; break;
        case Qt::Key_PageDown: virtualKey = VK_NEXT; break;
        case Qt::Key_Up: virtualKey = VK_UP; break;
        case Qt::Key_Down: virtualKey = VK_DOWN; break;
        case Qt::Key_Left: virtualKey = VK_LEFT; break;
        case Qt::Key_Right: virtualKey = VK_RIGHT; break;
        default:
            qWarning() << "Unsupported key:" << baseKey;
            return false;
        }
    }

    if (virtualKey == 0) {
        qWarning() << "Failed to map Qt key to Windows virtual key";
        return false;
    }

    return true;
#else
    Q_UNUSED(keySequence)
    Q_UNUSED(modifiers)
    Q_UNUSED(virtualKey)
    return false;
#endif
}

// 原生事件过滤器：捕捉并处理Windows热键消息
bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);

        // Handle WM_HOTKEY messages
        if (msg->message == WM_HOTKEY) {
            int hotkeyId = msg->wParam;

            // Emit the appropriate signal based on the hotkey ID
            if (hotkeyId == RECORDING_HOTKEY_ID) {
                emit recordingHotkeyPressed();
            } else if (hotkeyId == STOP_PLAYBACK_HOTKEY_ID) {
                emit stopPlaybackHotkeyPressed();
            }

            emit hotkeyPressed(hotkeyId);

            // Return true to indicate that we handled this message
            if (result) {
                *result = 0;
            }
            return true;
        }
    }
#else
    Q_UNUSED(eventType)
    Q_UNUSED(message)
    Q_UNUSED(result)
#endif

    return false;
}