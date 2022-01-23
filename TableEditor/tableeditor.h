#ifndef TABLEEDITOR_H
#define TABLEEDITOR_H

#include "tablebase.h"
#include <QDialog>
class TableEditor : public TableBase {
    Q_OBJECT
public:
    explicit TableEditor(const QString& Path, QWidget* parent = nullptr);

public:
    void readFile(const QString& path) override;
    void writeFile(const QString& path) override;

protected:
    const QString Split = "[|;|]";

    // QWidget interface
protected:
    void showEvent(QShowEvent* event) override;
};

#endif // TABLEEDITOR_H
