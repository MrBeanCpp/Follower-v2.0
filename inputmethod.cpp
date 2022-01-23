#include "inputmethod.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
InputMethod::InputMethod(const QString& path)
    : listPath(path)
{
    hkl = LoadKeyboardLayoutA("0x0409", KLF_ACTIVATE); //EN
    readListFile();
}

void InputMethod::checkAndSetEn()
{
    static char title[256];
    HWND hwnd = GetForegroundWindow();
    GetWindowTextA(hwnd, title, sizeof(title));
    if (isInList(title) && lastHwnd != hwnd)
        setEnMode(hwnd);
    lastHwnd = hwnd;
}

bool InputMethod::isInList(const QString& title)
{
    for (auto str : list)
        if (title.contains(str, Qt::CaseInsensitive))
            return true;
    return false;
}

void InputMethod::setEnMode(HWND hwnd)
{
    PostMessageA(hwnd, WM_INPUTLANGCHANGEREQUEST, (WPARAM)TRUE, (LPARAM)hkl);
}

void InputMethod::readListFile()
{
    QFile file(listPath);
    if (file.open(QFile::ReadWrite | QFile::Text)) {
        QTextStream text(&file);
        text.setCodec("UTF-8");
        while (!text.atEnd())
            list << text.readLine();
        file.close();
    }
}
