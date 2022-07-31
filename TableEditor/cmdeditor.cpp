#include "cmdeditor.h"
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QStringList>
#include <QTextStream>
#include <QWhatsThis>
#include <QInputDialog>
#include <QKeyEvent>
#include <QFileInfo>
CmdEditor::CmdEditor(const QString& Path, QWidget* parent)
    : TableEditor(Path, parent)
{
    setWindowTitle("Command-Editor");
    resize(DPI(QSize(1000, height())));
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

void CmdEditor::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_F && (event->modifiers() | Qt::ControlModifier)) { //Ctrl+F查找
        QInputDialog* d = new QInputDialog(this);
        d->setStyleSheet("QLineEdit{background:white;color:black;}"); //覆盖父类的qss
        d->setAttribute(Qt::WA_DeleteOnClose);
        d->setInputMode(QInputDialog::TextInput);

        static auto searchText = [=](const QString& text) {
            auto list = table->findItems(text, Qt::MatchContains);
            int rows = table->rowCount();
            for (int i = 0; i < rows; i++)
                table->setRowHidden(i, true);
            for (auto item : list) {
                int row = table->row(item);
                table->setRowHidden(row, false);
            }
        };

        connect(d, &QInputDialog::textValueChanged, searchText);
        connect(d, &QInputDialog::finished, [=]() {
            searchText(""); //恢复
        });
        connect(this, &CmdEditor::aboutToClose, d, &QInputDialog::close);
        d->show();
    }
}

void CmdEditor::closeEvent(QCloseEvent* event)
{
    emit aboutToClose(); //如果用的多的话 可以移至父类
    TableEditor::closeEvent(event); //调用父类函数
}

void CmdEditor::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event)
    if (dropItem) //对nullptr使用会卡顿
        dropItem->setText(dropPath);
    else{
        newLine();
        int row = table->currentRow();
        QString fileName = QFileInfo(dropPath).baseName();
        table->item(row, 2)->setText(dropPath);
        table->item(row, 0)->setText(fileName);
    }
}
