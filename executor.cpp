#include "executor.h"
#include <QDebug>
#include <QDesktopServices>
#include <QJSEngine>
#include <QRegExp>
#include <QTextStream>
#include <QUrl>
#include <windows.h>

Executor::Executor(QObject* parent)
    : QObject(parent)
{
    //QDir().mkdir(dirPath);
    //cmdEditor = new CmdEditor(cmdListPath);
    //noteEditor = new NoteEditor(noteListPath);
    readCmdList();
}

void Executor::readCmdList()
{
    if (sys->cmdEditor == nullptr) return;
    CmdEditor::TableList list = sys->cmdEditor->getContentList();
    cmdList.clear();
    for (QStringList& line : list)
        cmdList.append({ line.at(0), line.at(1), line.at(2), line.at(3) });
    std::sort(cmdList.begin(), cmdList.end(), [](const Command& a, const Command& b) {
        return a.code.length() < b.code.length();
    });
}

void Executor::openFile(const QString& filename, const QString& parameter, int nShowMode)
{
    if (filename == "") return; //""代表本工作目录
    int pos = filename.lastIndexOf('\\');
    QString dirPath = filename.left(pos); //缺省目录，防止找不到缺省文件//if pos == -1 return entire string
    //ShellExecuteA(0, "open", filename.toLocal8Bit().constData(), parameter.toLocal8Bit().constData(), dirPath.toLocal8Bit().constData(), nShowMode);
    ShellExecuteW(0, L"open", filename.toStdWString().c_str(), parameter.toStdWString().c_str(), dirPath.toStdWString().c_str(), nShowMode);
    //宽字符(Unicode)才能完美转换，否则可能编码错误
}

void Executor::editCmd()
{ //hide
    emit askHide();
    sys->cmdEditor->exec();
    readCmdList();
    emit askShow();
}

void Executor::editInputM()
{
    emit askHide();
    InputMethodEditor inputMEditor(inputMethodListPath);
    inputMEditor.exec();
    sys->inputM->readListFile(); //更新
    emit askShow();
}

void Executor::editNote()
{
    emit askHide();
    sys->noteEditor->exec();
    emit askShow();
}

bool Executor::isMatch(const QString& dst, const QString& str, Qt::CaseSensitivity cs) //以空格分割，乱序关键字匹配
{
    if (isBlank(str)) return false;
    if (symbol(dst) != symbol(str)) return false; //不同级别命令不比较
    //if ((str.at(0) == '#' && dst.at(0) != '#') || (str.at(0) != '#' && dst.at(0) == '#')) return false; //加入匹配size 符号类别二次匹配 getsymbol!!!!!!!!!!!!!!!!!!!
    bool extra = str.endsWith(' '); //(如："qt "->"qt"：false 以保证"qt code"快捷匹配)
    const QRegExp reg("\\W+"); //匹配至少一个[非字母数字]
    QStringList dstList = dst.simplified().split(reg, QString::SkipEmptyParts);
    QStringList strList = str.simplified().split(reg, QString::SkipEmptyParts);

    if (symbol(dst) == Inner_Cmd) dstList.push_front(InnerMark); //复原上一步清除的'#'
    if (symbol(str) == Inner_Cmd) strList.push_front(InnerMark);

    if (strList.empty()) return false;
    if (dstList.size() < strList.size() + extra) return false; //' '顶一个size

    for (const QString& _str : strList) { //存在多次匹配同一个单词问题(不过问题不大)
        bool flag = false;
        for (const QString& _dst : dstList) {
            if (_dst.indexOf(_str, 0, cs) == 0) {
                flag = true;
                break;
            }
        }
        if (!flag) return false;
    }
    return true;
}

bool Executor::isBlank(const QString& str)
{
    return str.simplified() == "";
}

Executor::CmdSymbol Executor::symbol(const QString& str)
{
    QChar ch = str.simplified().at(0);
    if (ch == InnerMark)
        return Inner_Cmd;
    else if (ch == JsMark)
        return Js_Cmd;
    else
        return Normal_Cmd;
}

void Executor::clearText()
{
    echoText.clear();
}

QString Executor::cleanPath(QString path)
{
    return path.replace('\"', "");
}

bool Executor::isExistPath(const QString& str)
{
    QString _str = cleanPath(str);
    return QFileInfo::exists(_str);
}

Executor::State Executor::run(const QString& code)
{
    clearText();
    if (code.isEmpty()) return NOCODE;

    if (symbol(code) == Js_Cmd) {
        QString body = code.simplified().mid(1);
        echoText = JsEngine.evaluate(body).toString();
        return JSCODE;
    }

    if (isExistPath(code)) {
        openFile(code);
        return PATH;
    }

    auto iter = std::find_if(cmdList.begin(), cmdList.end(), [=](const Command& cmd) { //模糊匹配
        return isMatch(cmd.code, code); //cmd.code.indexOf(code, 0, Qt::CaseInsensitive) == 0
    });
    auto iter_inner = std::find_if(innerCmdList.begin(), innerCmdList.end(), [=](const InnerCommand& cmd) {
        return isMatch(cmd.code, code); //cmd.code.indexOf(code, 0, Qt::CaseInsensitive) == 0
    });

    bool isFind = (iter != cmdList.end());
    bool isFind_inner = (iter_inner != innerCmdList.end());

    if (isFind) {
        Command cmd = *iter;
        openFile(cmd.filename, cmd.parameter);
        return CODE;
    } else if (isFind_inner) {
        InnerCommand cmd = *iter_inner;
        //(this->*(cmd.pfunc))();
        cmd.func(this); //指定对象
        return INNERCODE;
    } else { //最后的尝试//对人类最后的求爱
        openFile("cmd.exe", "/c " + code, SW_HIDE); //加上"/c"(close)，不然命令不会执行// "/k"(keep)也行
        return CMD;
    }
}

QStringList Executor::matchString(const QString& str, Qt::CaseSensitivity cs) //
{
    QStringList list;
    if (str == "") return list;

    if (symbol(str) == Js_Cmd) return list << "Inputing JavaScript Code...";

    if (isExistPath(str)) return list << "Maybe a Path...";

    for (const Command& cmd : cmdList)
        if (isMatch(cmd.code, str)) //忽略大小写//cmd.code.indexOf(str, 0, Qt::CaseInsensitive) == 0)
            list << cmd.code + cmd.extra;

    for (const InnerCommand& cmd : innerCmdList)
        if (isMatch(cmd.code, str)) { //忽略大小写
            QString str = cmd.showCode.isEmpty() ? cmd.code : cmd.showCode;
            if (!list.contains(str, cs)) //去重
                list << str;
        }
    return list;
}

bool Executor::hasText()
{
    return !echoText.isEmpty();
}

QString Executor::text()
{
    return echoText;
}
