#include "cmdeditor.h"
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QStringList>
#include <QTextStream>
#include <QWhatsThis>
CmdEditor::CmdEditor(const QString& Path, QWidget* parent)
    : TableEditor(Path, parent)
{
    setWindowTitle("Command-Editor");
    resize(1000, height());
    //table->setColumnWidth(1, width() * (7 / 12.0));//in resizeEvent
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(QStringList() << "Code"
                                                   << "Extra"
                                                   << "FileName"
                                                   << "Parameter");
    TableEditor::readFile(Path); //in showEvent,构造也要读，否则Executor一开始读不到
}

void CmdEditor::writeFile(const QString& path)
{
    simplifyPath();
    TableEditor::writeFile(path);
    QMessageBox::information(this, "Info", "Saved!");
}

void CmdEditor::whatsThisEvent()
{
    QMessageBox::about(this, "what's this?", "This function is based on the Windows API:\n"
                                             "\t[ShellExecute]\n"
                                             "Tip: remember '/c' in cmd");
}

void CmdEditor::simplifyPath()
{
    for (int i = 0; i < table->rowCount(); i++) {
        if (table->item(i, PathCol) == nullptr) continue;
        QString text = table->item(i, PathCol)->text().replace('\"', "");
        table->item(i, PathCol)->setText(text);
    }
}

void CmdEditor::resizeEvent(QResizeEvent* event)
{
    TableEditor::resizeEvent(event);
    table->setColumnWidth(PathCol, width() * (7 / 12.0));
}
