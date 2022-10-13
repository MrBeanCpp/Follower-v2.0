#ifndef EXECUTOR_H
#define EXECUTOR_H
#include "TableEditor/cmdeditor.h"
#include "TableEditor/inputmethodeditor.h"
#include "TableEditor/noteeditor.h"
#include "Utils/hook.h"
#include "Utils/path.h"
#include "Utils/systemapi.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QJSEngine>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>
#include <functional>
#include <iostream>
struct Command {
    QString code;
    QString extra; //额外提示（不参与匹配）
    QString filename;
    QString parameter;
};
class Executor;
struct InnerCommand {
    QString code;
    QString showCode; //实际显示code，""时默认与code相同
    //void (Executor::*pfunc)(void);
    std::function<void(Executor*)> func; //std::function无补全，裂开
};

class Executor : public QObject //×持有各种资源，如note,inputM
{ //NO!SystemAPI才应该持有资源
    Q_OBJECT
public:
    friend class CodeEditor;
    enum State {
        NOCODE,
        CODE,
        INNERCODE,
        CMD,
        JSCODE,
        TRANSLATE,
        PATH
    };
    Executor(QObject* parent = nullptr);
    State run(const QString& code, bool isWithExtra = false);
    QList<QPair<QString, QString>> matchString(const QString& str, State* state = nullptr, Qt::CaseSensitivity cs = Qt::CaseInsensitive); //cmdList中匹配的命令(模糊查询)
    bool hasText(void);
    QString text(void);
    QList<Command> getCMDList(void);

public:
    const QString dirPath = Path::dirPath();
    const QString cmdListPath = Path::cmdList();
    const QString inputMethodListPath = Path::inputMethodList();
    const QString noteListPath = Path::noteList();
    const QString iniFilePath = Path::iniFile();
    const QString runTimesDataPath = Path::runTimesData();

private:
    enum CmdSymbol {
        Inner_Cmd,
        Normal_Cmd,
        Js_Cmd,
        Trans_Cmd,
        Path_Cmd
    };

    const QChar InnerMark = '#';
    const QChar JsMark = '@';
    const QChar TransMark = '!';

    QList<Command> cmdList;
    QList<InnerCommand> innerCmdList = { //CaseInsensitive
        { "#Edit cmd", "", &Executor::editCmd },
        { "#Edit note", "", &Executor::editNote },
        { "#Edit InputMethod", "", &Executor::editInputM },
        { "#Teleport [On]", "#Teleport [On/Auto/Off]", [](Executor* exe) { emit exe->changeTeleportMode(1); } },
        { "#Teleport [Auto]", "#Teleport [On/Auto/Off]", [](Executor* exe) { emit exe->changeTeleportMode(0); } },
        { "#Teleport [Off]", "#Teleport [On/Auto/Off]", [](Executor* exe) { emit exe->changeTeleportMode(-1); } },
        { "#Open [Core-Info]", "", [](Executor* exe) { exe->openFile(exe->dirPath); } },
        { "#Lock-[Keyboard]", "", [](Executor* exe) {Q_UNUSED(exe); Hook::setKeyboardHook(); } },
        { "#quit", "", [](Executor* exe) { Q_UNUSED(exe); qApp->quit(); } }
    };
    QJSEngine JsEngine;
    QString echoText;
    QMap<QString, int> runTimesMap;

    //CmdEditor* cmdEditor = nullptr;
    //NoteEditor* noteEditor = nullptr;

private:
    void readCmdList(void);
    void openFile(const QString& filename, const QString& parameter = "", int nShowMode = SW_SHOW);
    void editCmd(void);
    void editInputM(void);
    void editNote(void);
    bool isMatch(const QString& dst, const QString& str, Qt::CaseSensitivity cs = Qt::CaseInsensitive); //Unordered
    inline bool isBlank(const QString& str);
    inline CmdSymbol symbol(const QString& str);
    void clearText(void);
    QString cleanPath(QString path); //去掉双引号
    bool isExistPath(const QString& str);

signals:
    void askHide(void);
    void askShow(void);
    void changeTeleportMode(int);
};
#endif // EXECUTOR_H
