#include "codeeditor.h"
#include "hook.h"
#include <QClipboard>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QProcess>
#include <QString>
#include <windows.h>
#include <QDirModel>
#include <QRegExp>
CodeEditor::CodeEditor(int width, int height, QWidget* parent)
    : QLineEdit(parent), label(new QLabel(parent)), lw(new CMDListWidget(parent)), normalWidth(width), normalHeight(height)
{
    setContextMenuPolicy(Qt::NoContextMenu);
    setMaxLength(TextLimit); //设置长度限制

    setStyleSheet("QLineEdit{background-color:rgb(15,15,15);color:rgb(225,225,225);font-family:Consolas;font-size:14pt}");

    label->setStyleSheet("QLabel{background-color:rgb(15,15,15);color:rgb(200,200,200);font-family:Consolas}");
    label->hide();

    lw->hide();

    connect(this, &CodeEditor::textEdited, this, &CodeEditor::textEdit);
    connect(this, &CodeEditor::returnPressed, this, &CodeEditor::returnPress);

    connect(lw, &CMDListWidget::itemActivedEx, this, &CodeEditor::returnPress);

    comp = new QCompleter(new QDirModel(this), parent); //如果completer的父对象是lineEdit 则setCompleter(nullptr)会delete completer(参照Qt源码)
    comp->setCompletionMode(QCompleter::InlineCompletion);

    //用于在触发补全时(setText 不触发textEdit) 调整size
    connect(comp, QOverload<const QString&>::of(&QCompleter::highlighted), [=](const QString& text) {
        adjustWholeSize(text); //放在textChanged也行 但为了提升性能 不做重复adjust (text()比较滞后)
    });

    auto cmdList = executor.getCMDList();
    for (const auto& cmd : cmdList)
        iconPro.addCache(cmd.filename); //初始化缓存
}

void CodeEditor::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event)
    this->clear();
    label->hide();
    lw->hide();
    lw->clear();
    emit reportResize(normalWidth, normalHeight); //瞬间归位 防止拖沓
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
    lw->resize(width(), lw->height());
}

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    int key = event->key();
    int modifiers = event->modifiers();

    if (key == Qt::Key_Space && text().isEmpty()) //文本为空时，不允许空格(没必要)
        return;
    else if (key == Qt::Key_Up) {
        if (modifiers & Qt::ControlModifier) { //上条记录
            QString text = pastCodeList.past();
            setText(text);
            textEdit(text);
        } else { //选择上一项
            if (lw->isVisible())
                lw->selectPre();
        }
    } else if (key == Qt::Key_Down) {
        if (modifiers & Qt::ControlModifier) { //下条记录
            QString text = pastCodeList.next();
            setText(text);
            textEdit(text);
        } else { //选择下一项
            if (lw->isVisible())
                lw->selectNext();
        }
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (completer() && selectedText() != "") { //手动在hasSelectedText的时候阻止run
            QString str = text();
            if (str.endsWith('\\')) { //目录需要去掉'\'
                str.chop(1);
                setText(str);
            }
            end(false);
            deselect();
            return;
        }
    }
    return QLineEdit::keyPressEvent(event);
}

void CodeEditor::showLabel(const QString& text)
{
    hideList(false);
    if (!label->isVisible()) {
        label->move(this->x(), this->geometry().bottom() + Margin);
        label->show();
    }
    label->setText(text);
    label->adjustSize(); //迫使size自适应生效//setFixedWidth会妨碍width的自动调整
    adjustWholeSize();
}

void CodeEditor::hideLabel(bool isAdjustSize)
{
    if (label->isVisible()) {
        label->hide();
        if (isAdjustSize)
            adjustWholeSize();
    }
}

void CodeEditor::showList(const CodeEditor::IconStrList& list)
{
    hideLabel(false);
    if (!lw->isVisible()) {
        lw->move(this->x(), this->geometry().bottom() + Margin);
        lw->show();
    }
    lw->addIconItems(list);
    adjustWholeSize();
}

void CodeEditor::hideList(bool isAdjustSize)
{
    if (lw->isVisible()) {
        lw->hide();
        if (isAdjustSize)
            adjustWholeSize();
    }
}

void CodeEditor::hideDisplay()
{
    hideLabel(false);
    hideList(false);
    adjustWholeSize();
}

void CodeEditor::textEdit(const QString& text) //不包括setText clear等代码方式的edit
{
    if (text.length() >= maxLength()) { //在==后就不会触发textEdit
        showLabel("#Warning: too many words#");
        return;
    }

    if (!text.isEmpty()) {
        QList<QPair<QString, QString>> list = executor.matchString(text);
        setCompleter(text.contains(':') ? comp : nullptr); //设置路径补全(检测':'防止盘符字母和Code混淆)

        if (list.empty()) {
            showLabel(executor.hasText() ? executor.text() : "No match code");
        } else {
            IconStrList itemList;
            for (const auto& p : list) {
                const QString& path = p.second;
                itemList << qMakePair(iconPro.icon(path), p.first);
            }
            showList(itemList);
        }
    } else
        hideDisplay();
}

void CodeEditor::adjustWholeSize(const QString& str)
{
    int width = QFontMetrics(font()).horizontalAdvance(str == "" ? text() : str) + 10; //考虑边框
    int height = normalHeight;
    width = qMax(width, normalWidth);

    if (label->isVisible()) {
        height = normalHeight + label->height() + Margin;
        width = qMax(width, label->width());
        label->resize(width, label->height());
    }

    if (lw->isVisible()) {
        width = qMax(width, lw->width());
        height = normalHeight + lw->height() + Margin;
        lw->resize(width, lw->height());
    }

    emit reportResize(width, height);
}

void CodeEditor::returnPress()
{
    if (text() == "") {
        if (placeholderText() == Holder_note)
            executor.editNote();
        hideDisplay();
    } else {
        pastCodeList << text(); //加入历史记录

        Executor::State state;
        if (lw->isVisible())
            state = executor.run(lw->selectedText(), true); //带上extra一起匹配
        else
            state = executor.run(text());

        if (executor.hasText()) {
            QString echoText = executor.text();
            if (state == Executor::JSCODE) {
                setText(executor.JsMark); //setText不会触发edit信号（这也算好处）
                QApplication::clipboard()->setText(echoText); //
                showLabel(echoText + "\n-Clipboard-"); //自动拷贝进剪切板
            } else if (state == Executor::TRANSLATE) {
                static auto isEn = [](const QString& text) -> bool {
                    static QRegExp reg("[A-Za-z0-9 ,.]+");
                    return reg.exactMatch(text);
                };

                setText(executor.TransMark);
                showLabel("Translate: " + echoText);
                request.translate(
                    echoText, [=](const QString& text) {
                        if (isVisible() == false) return;

                        if (text.isEmpty())
                            showLabel("[WARN]Error or Timeout");
                        else
                            showLabel(echoText + ":\n" + text);
                    },
                    2000,
                    "auto",
                    isEn(echoText) ? "zh" : "en"); //自动识别
            }
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
