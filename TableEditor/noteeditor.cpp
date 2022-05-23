#include "noteeditor.h"
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTimeEdit>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMessageBox>
#include <QScrollBar>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <qmath.h>
const QStringList NoteEditor::RepeatMode = { "None", "Minute", "Hour", "Day", "Month" };

NoteEditor::NoteEditor(const QString& path, QWidget* parent)
    : TableEditor(path, parent)
{
    setWindowTitle("Note-Editor");
    table->setColumnCount(Col);
    table->setHorizontalHeaderLabels(QStringList() << "Notes"
                                                   << "Remind Time"
                                                   << "Repeat");
    resize(DPI(QSize(950, height())));
    vScrollBar = table->verticalScrollBar();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    //table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    btn_clear = new QToolButton(this);
    btn_clear->resize(DPI(QSize(20, 20)));
    btn_clear->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));

    connect(sys->gTimer, &GapTimer::timeout, this, &NoteEditor::checkRemindTime);

    connect(table->horizontalHeader(), &QHeaderView::geometriesChanged, [=]() {
        adjustColWidth(); //调整列宽
    });
    connect(btn_clear, &QToolButton::clicked, this, &NoteEditor::clear);
    readFile(path);
}

void NoteEditor::setDateTimeEdit(int row, int col, const QDateTime& dateTime)
{
    QDateTimeEdit* edit = new QDateTimeEdit(dateTime);
    edit->setDisplayFormat(DateTimeFormat);
    edit->setCurrentSectionIndex(2);
    edit->setCalendarPopup(true);
    edit->setFocusPolicy(Qt::StrongFocus); //防止滚轮窃取焦点
    auto update_color = [=](const QDateTime& dateTime) {
        edit->setStyleSheet(dateTime < QDateTime::currentDateTime() ? "QDateTimeEdit{color:grey}" : "QDateTimeEdit{color:white}"); //过期灰色
    };
    update_color(dateTime);
    table->setCellWidget(row, col, edit);
    edit->installEventFilter(this);
    connect(edit, &QDateTimeEdit::dateTimeChanged, update_color);
}

void NoteEditor::setRepeatBox(int row, int col, const QStringList& list, int index)
{
    QComboBox* box = new QComboBox;
    box->addItems(list);
    box->setCurrentIndex(index);
    auto update_color = [](QComboBox* box) {
        box->setStyleSheet(box->currentIndex() == 0 ? "QComboBox{color:grey}" : "QComboBox{color:white}"); //过期灰色
    };
    update_color(box);
    table->setCellWidget(row, col, box);
    box->installEventFilter(this);
    connect(box, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) { Q_UNUSED(index); update_color(box); });
}

QDateTimeEdit* NoteEditor::dateTimeEdit(int row, int col)
{
    return static_cast<QDateTimeEdit*>(table->cellWidget(row, col));
}

QComboBox* NoteEditor::comboBox(int row, int col)
{
    return static_cast<QComboBox*>(table->cellWidget(row, col));
}

void NoteEditor::checkRemindTime()
{
    for (int i = 0; i < table->rowCount(); i++) {
        QDateTime dateTime = getRepeatDate(dateTimeEdit(i)->dateTime(), comboBox(i)->currentText());
        if (sys->gTimer->inGap(dateTime)) {
            QTableWidgetItem* item = table->item(i, 0);
            if (item != nullptr && !isBlank(item->text()))
                sys->sysTray->showMessage("Time Reminder", item->text());
        }
    }
}

void NoteEditor::adjustColWidth()
{
    //table->setColumnWidth(0, table->width() - table->verticalHeader()->width() - DateColWidth - RepeatColWidth - (hasVScrollBar ? vScrollBar->width() : 0));
    table->setColumnWidth(0, table->horizontalHeader()->width() - DateColWidth - RepeatColWidth);
    table->setColumnWidth(1, DateColWidth);
}

QDateTime NoteEditor::getRepeatDate(const QDateTime& dateTime, const QString& repeatMode)
{
    int index = RepeatMode.indexOf(repeatMode);
    if (index == -1) return QDateTime();
    QDateTime now = QDateTime::currentDateTime();
    if (index == 0)
        return dateTime;
    else if (index == 1) {
        int sec = dateTime.secsTo(now);
        return dateTime.addSecs(qCeil(sec / 60.0) * 60);
    } else if (index == 2) {
        int sec = dateTime.secsTo(now);
        return dateTime.addSecs(qCeil(sec / 3600.0) * 3600);
    } else if (index == 3) {
        return QDateTime(QDate::currentDate(), dateTime.time());
    } else if (index == 4) {
        QDate date = QDate::currentDate();
        return QDateTime(QDate(date.year(), date.month(), dateTime.date().day()), dateTime.time());
    }
    return QDateTime();
}

QDateTime NoteEditor::zeroSecond(QDateTime dateTime)
{
    QTime time = dateTime.time();
    dateTime.setTime(QTime(time.hour(), time.minute(), 0));
    return dateTime;
}

void NoteEditor::readFile(const QString& path)
{
    clear();
    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream text(&file);
        text.setCodec("UTF-8");
        int row = 0;
        while (!text.atEnd()) {
            QStringList list = text.readLine().split(Split);
            if (list.size() != Col) {
                QMessageBox::critical(this, "ERR", "[in NoteEditor],some row's col != " + QString::number(Col));
                continue;
            }
            table->setRowCount(row + 1);

            table->setItem(row, 0, new QTableWidgetItem(list[0].simplified()));

            QString dateTime = list[1].simplified();
            setDateTimeEdit(row, 1, QDateTime::fromString(dateTime, DateTimeFormat));

            QString repeatMode = list[2].simplified();
            int index = RepeatMode.indexOf(repeatMode);
            setRepeatBox(row, 2, RepeatMode, index);

            row++;
        }
        file.close();
    }
    oldList = getContentList(); //更新oldList
}

void NoteEditor::writeFile(const QString& path)
{
    int row = table->rowCount();
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Text)) return;
    QTextStream text(&file);
    text.setCodec("UTF-8");
    for (int i = 0; i < row; i++) {
        QString note, dateTime, repeatMode;
        QTableWidgetItem* item = table->item(i, 0);
        QDateTimeEdit* cell = dateTimeEdit(i, 1);
        QComboBox* box = comboBox(i, 2);

        if (isItemValid(item))
            note = item->text();
        if (cell != nullptr && !isBlank(cell->text()))
            dateTime = cell->text();
        if (box != nullptr)
            repeatMode = box->currentText();
        else
            repeatMode = RepeatMode.at(0);

        if (note.isEmpty()) {
            table->removeRow(i), --row, --i;
            continue;
        }
        text << note << ' ' << Split << ' ' << dateTime << ' ' << Split << ' ' << repeatMode << '\n';
    }
    file.close();
    oldList = getContentList(); //更新oldList
    QMessageBox::information(this, "Info", "Notes have been Saved!");
}

void NoteEditor::newLine()
{
    TableEditor::newLine();
    //QDateTime dateTime = QDateTime::currentDateTime().addSecs(60);
    //QTime time = dateTime.time();
    //dateTime.setTime(QTime(time.hour(), time.minute(), 0));
    setDateTimeEdit(table->currentRow(), 1, zeroSecond(QDateTime::currentDateTime().addSecs(60))); //add 1 min and set second 0，防止高亮错误

    setRepeatBox(table->currentRow(), 2);
}

TableEditor::TableList NoteEditor::getContentList()
{
    TableList list = TableEditor::getContentList(); //基类无法读取cell & box
    for (int i = 0, index = 0; i < table->rowCount(); i++)
        if (isItemValid(table->item(i, 0))) { //排除空Note，与基类保持一致，防止下标越界
            list[index][1] = dateTimeEdit(i, 1)->text();
            list[index][2] = comboBox(i, 2)->currentText();
            index++;
        }
    return list;
}

bool NoteEditor::exChangeRow(int row, int NewRow)
{
    if (TableEditor::exChangeRow(row, NewRow) == false) return false;
    QDateTime dateTime = dateTimeEdit(row)->dateTime();
    QDateTime NewDateTime = dateTimeEdit(NewRow)->dateTime();
    setDateTimeEdit(row, 1, NewDateTime);
    setDateTimeEdit(NewRow, 1, dateTime);

    QString repeatMode = comboBox(row)->currentText();
    QString NewRepeatMode = comboBox(NewRow)->currentText();
    comboBox(row)->setCurrentText(NewRepeatMode);
    comboBox(NewRow)->setCurrentText(repeatMode);
    return true;
}

void NoteEditor::whatsThisEvent()
{
    QMessageBox::about(this, "what's this?", "Emergency Notes");
}

bool NoteEditor::isEmpty(const QString& path)
{
    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream text(&file);
        text.skipWhiteSpace();
        bool ret = text.atEnd();
        file.close();
        return ret;
    } else
        return true;
}

bool NoteEditor::isEmpty()
{
    return isEmpty(path);
}

void NoteEditor::resizeEvent(QResizeEvent* event)
{
    TableEditor::resizeEvent(event);
    btn_clear->move(ui->btn_add->x(), DPI(10));
}

bool NoteEditor::eventFilter(QObject* watched, QEvent* event)
{
    if (watched->inherits("QDateTimeEdit")) {
        QDateTimeEdit* edit = static_cast<QDateTimeEdit*>(watched);
        if (event->type() == QEvent::Wheel) {
            if (!edit->hasFocus()) //无焦点时，不允许更改DateTime
                return true;
        } else if (event->type() == QEvent::KeyRelease) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_R) { //reset to now
                QDateTime dateTime = zeroSecond(QDateTime::currentDateTime());
                edit->setDateTime(dateTime);
                emit edit->dateTimeChanged(dateTime); //强行触发color检测（秒数本来就为0，同一分钟内zeroSecond()不会触发dateTimeChanged）
            }
        }
    } else if (watched->inherits("QComboBox")) {
        if (event->type() == QEvent::KeyPress)
            return TableEditor::eventFilter(watched, event); //保证快捷键
    }
    return TableEditor::eventFilter(watched, event);
}
