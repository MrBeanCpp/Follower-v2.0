#include "codeeditor.h"
#include "hook.h"
#include <QClipboard>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QProcess>
#include <QString>
#include <windows.h>
CodeEditor::CodeEditor(int width, int height, QWidget* parent)
    : QLineEdit(parent), label(new QLabel(parent)), normalWidth(width), normalHeight(height)
{
    setContextMenuPolicy(Qt::NoContextMenu);

    setStyleSheet("QLineEdit{background-color:rgb(15,15,15);color:rgb(225,225,225);font-family:Consolas;font-size:14pt}");

    label->setStyleSheet("QLabel{background-color:rgb(15,15,15);color:rgb(200,200,200);font-family:Consolas}");
    label->hide();

    connect(this, &CodeEditor::textEdited, this, &CodeEditor::textEdit);
    connect(this, &CodeEditor::returnPressed, this, &CodeEditor::returnPress);

    //clipboard = qApp->clipboard(); //系统剪切板
}

void CodeEditor::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event)
    label->hide();
}

void CodeEditor::focusInEvent(QFocusEvent* event)
{
    sys->inputM->setEnMode(HWND(winId()));
    QLineEdit::focusInEvent(event);
}

void CodeEditor::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)
    label->resize(width(), label->height());
}

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space && text() == "") //文本为空时，不允许空格(没必要)
        return;
    else if (event->matches(QKeySequence::Paste)) { //阻止过长粘贴，防止卡住
        QString text = this->text() + QApplication::clipboard()->text();
        if (text.length() > TextLimit) {
            setText(text.left(TextLimit));
            showLabel("#Warning: too many words#");
            return;
        }
    } else if (event->key() == Qt::Key_Up) { //上条记录
        QString text = pastCodeList.past();
        setText(text);
        textEdit(text);
    } else if (event->key() == Qt::Key_Down) { //下条记录
        QString text = pastCodeList.next();
        setText(text);
        textEdit(text);
    }
    return QLineEdit::keyPressEvent(event);
}

void CodeEditor::showLabel(const QString& text)
{
    if (!label->isVisible()) {
        label->show();
        label->move(this->x(), this->y() + this->height() + Margin);
    }
    label->setText(text);
    label->adjustSize(); //迫使size自适应生效//setFixedWidth会妨碍width的自动调整
    adjustWholeSize();
}

void CodeEditor::hideLabel()
{
    label->hide();
    adjustWholeSize();
}

QString CodeEditor::listToStr(const QStringList& strList)
{
    QString res;
    for (const QString& str : strList)
        (res += str) += '\n';
    res.remove(res.length() - 1, 1); //移除最后一个\n
    return res;
}

void CodeEditor::textEdit(const QString& text)
{
    if (text.length() > TextLimit) {
        setText(text.left(TextLimit));
        adjustWholeSize();
        return;
    }
    if (text != "") {
        QString matchCmd = listToStr(executor.matchString(text));
        if (matchCmd == "") matchCmd = "No match code";
        showLabel(matchCmd);
    } else
        hideLabel();
}

void CodeEditor::adjustWholeSize()
{
    int width = QFontMetrics(font()).horizontalAdvance(text()) + 10; //考虑边框
    int height = normalHeight;

    if (label->isVisible()) {
        label->adjustSize(); //激活自适应
        width = qMax(width, label->width());
        height = normalHeight + label->height() + Margin;
    }

    width = qMax(width, normalWidth);
    label->resize(width, label->height());
    emit reportResize(width, height);
}

void CodeEditor::returnPress()
{
    if (text() == "") {
        if (placeholderText() == Holder_note)
            executor.editNote();
        hideLabel();
    } else {
        pastCodeList << text(); //加入历史记录
        executor.run(text());
        if (executor.hasText()) {
            QString echoText = executor.text();
            showLabel(echoText + "\n-Clipboard-"); //自动拷贝进剪切板
            QApplication::clipboard()->setText(echoText); //
            clear();
            adjustWholeSize();
        } else {
            showLabel("#Command Over#");
            emit returnWithoutEcho();
        }
    }
}

void CodeEditor::silent()
{
    hide();
    setEnabled(false);
}

void CodeEditor::wake()
{
    Hook::unHook();
    if (!sys->noteEditor->isEmpty())
        setPlaceholderText(Holder_note);
    else
        setPlaceholderText("");
    show();
    setEnabled(true);
}
