#ifndef HOOK_H
#define HOOK_H
#include <windows.h>
class Hook
{
public:
    Hook() = delete;
    static void setKeyboardHook(void);
    static void setMouseHook(void);
    static void unHook(void);
    static bool isKeyLock(void);

private:
    static HHOOK h_key, h_mouse;
    static LRESULT CALLBACK keyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam);
};

#endif // HOOK_H
