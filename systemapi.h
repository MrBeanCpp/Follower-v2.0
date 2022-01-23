#ifndef SYSTEMAPI_H
#define SYSTEMAPI_H

#include "gaptimer.h"
#include "inputmethod.h"
#include "path.h"
#include <QObject>
#include <QSystemTrayIcon>
class CmdEditor; //防止头文件相互包含 但可以在.cpp中
class NoteEditor;

class SystemAPI : public QObject { //公共唯一资源
    Q_OBJECT
public:
    SystemAPI(QObject* parent = nullptr);
    ~SystemAPI();

private:
    static void Init(void);
    static void Init_SystemTray(void);
    static void Init_InputMethod(void);
    static void Init_GapTimer(void);
    static void Init_CmdEditor(void);
    static void Init_NoteEditor(void);

public:
    static QSystemTrayIcon* sysTray;
    static InputMethod* inputM;
    static GapTimer* gTimer;
    static CmdEditor* cmdEditor;
    static NoteEditor* noteEditor;
};
extern SystemAPI* sys;
#endif // SYSTEMAPI_H
