#include "tableeditor.h"
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QStringList>
#include <QTextStream>
#include <QWhatsThis>
TableEditor::TableEditor(const QString& Path, QWidget* parent)
    : TableBase(Path, parent)
{
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->verticalHeader()->setMinimumHeight(DPI(20));
}

void TableEditor::readFile(const QString& path)
{
    clear();
    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream text(&file);
        text.setCodec("UTF-8");
        int row = 0;
        while (!text.atEnd()) {
            QString line = text.readLine();
            QStringList list = line.split(Split);
            int col = 0;
            table->setRowCount(row + 1);
            for (QString& str : list) {
                str = str.trimmed(); //去除多余空白字符//simplify可能导致内部空格丢失导致错误
                QTableWidgetItem* item = new QTableWidgetItem(str);
                table->setItem(row, col++, item);
            }
            row++;
        }
        file.close();
    }
    oldList = getContentList(); //更新oldList
}

void TableEditor::writeFile(const QString& path)
{
    int row = table->rowCount(), col = table->columnCount();
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Text)) return;
    QTextStream text(&file);
    text.setCodec("UTF-8");
    for (int i = 0; i < row; i++) {
        QStringList list;
        for (int j = 0; j < col; j++) {
            QTableWidgetItem* item = table->item(i, j);
            if (isItemValid(item)) //无内容即为null，在readFile中未setItem，调用nullptr指针出错
                list << item->text();
            else
                list << "";
        }
        if (list.count("") == list.size()) {
            table->removeRow(i), --row, --i;
            continue;
        }
        text << list.join(' ' + Split + ' ') << '\n';
    }
    file.close();
    oldList = getContentList(); //更新oldList
}

void TableEditor::showEvent(QShowEvent* event)
{
    Q_UNUSED(event)
    readFile(path);
}
