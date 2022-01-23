#ifndef CMDEDITOR_H
#define CMDEDITOR_H

#include "tableeditor.h"
#include <QDialog>
class CmdEditor : public TableEditor {
    Q_OBJECT
public:
    explicit CmdEditor(const QString& Path, QWidget* parent = nullptr);

public:
    void writeFile(const QString& path) override;
    void whatsThisEvent(void) override;

private:
    void simplifyPath(void); //去除路径引号

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    const int PathCol = 2;
};

#endif // CMDEDITOR_H
