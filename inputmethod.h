#ifndef INPUTMETHOD_H
#define INPUTMETHOD_H

#include "path.h"
#include <QApplication>
#include <QList>
#include <QString>
#include <QStringList>
#include <windows.h>
class InputMethod
{
public:
    InputMethod(const QString& path = Path::inputMethodList());
    void setEnMode(HWND hwnd);
    void checkAndSetEn(void);
    void readListFile(void); //在edit之后会重新读取(in executor.cpp)

private:
    HKL hkl;
    HWND lastHwnd = NULL;
    const QString listPath;
    QStringList list;

private:
    inline bool isInList(const QString& title);
    bool isWindowTitleInList(const QString& title);
    bool isProcessNameInList(const QString& name);
    DWORD getForeWindowPID(HWND hwnd = NULL);
    QString getProcessNameByPID(DWORD PID);
    QString getWindowTitle(HWND hwnd = NULL);
};

#endif // INPUTMETHOD_H
