#ifndef NOTEEDITOR_H
#define NOTEEDITOR_H

#include "systemapi.h"
#include "tableeditor.h"
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFileSystemWatcher>
#include <QString>
#include <QWidget>
class NoteEditor : public TableEditor {
    Q_OBJECT
public:
    explicit NoteEditor(const QString& path, QWidget* parent = nullptr);

private:
    QToolButton* btn_clear = nullptr;
    const int Col = 3;
    const int DateColWidth = DPI(128);
    const int RepeatColWidth = DPI(70);
    const QString DateTimeFormat = "yyyy-MM-dd hh:mm";
    QScrollBar* vScrollBar;
    static const QStringList RepeatMode;

private:
    void setDateTimeEdit(int row, int col, const QDateTime& dateTime = QDateTime::currentDateTime());
    void setRepeatBox(int row, int col, const QStringList& list = RepeatMode, int index = 0);
    QDateTimeEdit* dateTimeEdit(int row, int col = 1);
    QComboBox* comboBox(int row, int col = 2);
    void checkRemindTime(void);
    void adjustColWidth();
    QDateTime getRepeatDate(const QDateTime& dateTime, const QString& repeatMode);
    QDateTime zeroSecond(QDateTime dateTime);

public:
    void readFile(const QString& path) override;
    void writeFile(const QString& path) override;
    void newLine(void) override;
    TableList getContentList(void) override;
    bool exChangeRow(int row, int NewRow) override;
    void whatsThisEvent(void) override;

    static bool isEmpty(const QString& path);
    bool isEmpty(void);

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent* event) override;

    // QObject interface
public:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // NOTEEDITOR_H
