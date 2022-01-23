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
    void readListFile(void);

private:
    HKL hkl;
    HWND lastHwnd = NULL;
    const QString listPath;
    QStringList list;

private:
    inline bool isInList(const QString& title);
};

#endif // INPUTMETHOD_H
