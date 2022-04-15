#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include "executor.h"
#include "systemapi.h"
#include <QLabel>
#include <QLineEdit>
#include <QWidget>

struct PastCodeList {
    PastCodeList()
        : lineLimit(10), index(0) {}
    explicit PastCodeList(int LineLimit)
        : lineLimit(LineLimit), index(0) {}
    void setLineLimit(int LineLimit) { lineLimit = LineLimit; }
    PastCodeList& operator<<(const QString& str)
    {
        if (list.empty() || list.back() != str) { //短路求值保证list有效访问
            if (list.size() >= lineLimit) list.removeFirst();
            list << str;
        }
        index = list.size();
        return *this;
    }
    QString past(void)
    {
        index--;
        fixIndex();
        if (!isValid(index)) return "";
        return list.at(index);
    }
    QString next(void)
    {
        index++;
        fixIndex();
        if (!isValid(index)) return "";
        return list.at(index);
    }
    bool isValid(int Index)
    {
        if (Index < 0 || list.empty()) return false;
        return Index < list.size();
    }
    void fixIndex(void)
    {
        if (index < 0) index = 0;
        if (index >= list.size()) index = list.size() > 0 ? list.size() - 1 : 0;
    }

private:
    QStringList list;
    int lineLimit;
    int index;
};

class Widget;
class CodeEditor : public QLineEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(int width, int height, QWidget* parent = nullptr);
    friend class Widget;
signals:
    void reportResize(int width, int height); //-1 means Automatic
    //只报告自身内部Size，不理睬主窗体Margin(所谓，两耳不闻窗外事)
    void returnWithoutEcho(void); //按下回车，且无回显的Text

private:
    Executor executor;
    QLabel* label;
    //QClipboard* clipboard;

    const int normalWidth; //LineEditor宽度
    const int normalHeight;
    const int Margin = 5; //内部Margin，与主窗体无关
    const int TextLimit = 50;
    const QString Holder_note = "note?";

    PastCodeList pastCodeList { 10 }; //code历史记录//不能用()存在歧义(函数声明or变量声明)

    // QWidget interface
protected:
    void hideEvent(QHideEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void showLabel(const QString& text);
    void hideLabel(void);
    QString listToStr(const QStringList& strList);
    void textEdit(const QString& text);
    void adjustWholeSize();
    void returnPress(void);

public:
    void silent(void);
    void wake(void);
};

#endif // CODEEDITOR_H