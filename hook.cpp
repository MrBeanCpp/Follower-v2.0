#include "hook.h"
#include "systemapi.h"
#include <QDebug>
HHOOK Hook::h_key = nullptr;
HHOOK Hook::h_mouse = nullptr;

void Hook::setKeyboardHook()
{
    h_key = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProc, GetModuleHandle(NULL), 0);
    sys->sysTray->showMessage("Hook Tip", "KeyBoard-Lock\n(Click Follower to Cancel)");
}

void Hook::setMouseHook()
{
    h_mouse = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, GetModuleHandle(NULL), 0);
    sys->sysTray->showMessage("Hook Tip", "Mouse-Lock\n(Click Follower to Cancel)");
}

void Hook::unHook()
{
    if (h_key != nullptr) {
        UnhookWindowsHookEx(h_key);
        h_key = nullptr;
        sys->sysTray->showMessage("Hook Tip", "keyBoard-UnLock");
    }
    if (h_mouse != nullptr) {
        UnhookWindowsHookEx(h_mouse);
        h_mouse = nullptr;
        sys->sysTray->showMessage("Hook Tip", "Mouse-UnLock");
    }
}

bool Hook::isKeyLock()
{
    return h_key != nullptr;
}

LRESULT Hook::keyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    Q_UNUSED(nCode)
    Q_UNUSED(wParam)
    Q_UNUSED(lParam)
    qDebug() << "keyBoard Hook:" << ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
    return true;
}

LRESULT Hook::mouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    Q_UNUSED(nCode)
    Q_UNUSED(wParam)
    Q_UNUSED(lParam)
    qDebug() << "mouse Hook" << ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
    return true;
}
