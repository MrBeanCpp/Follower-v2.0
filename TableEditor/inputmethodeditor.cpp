#include "inputmethodeditor.h"
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
InputMethodEditor::InputMethodEditor(const QString& path, QWidget* parent)
    : TableEditor(path, parent)
{
    setWindowTitle("InputMethod-Editor");
    table->setColumnCount(1);
    table->setHorizontalHeaderLabels(QStringList() << "String in the Title(to turn to En)");
    TableEditor::readFile(path); //强调未重写
}

void InputMethodEditor::writeFile(const QString& path)
{
    TableEditor::writeFile(path);
    QMessageBox::information(this, "Info", "Saved!");
}
