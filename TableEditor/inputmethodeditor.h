#ifndef INPUTMETHODEDITOR_H
#define INPUTMETHODEDITOR_H

#include "tableeditor.h"
#include <QString>
#include <QWidget>
class InputMethodEditor : public TableEditor {
    Q_OBJECT
public:
    explicit InputMethodEditor(const QString& path, QWidget* parent = nullptr);

public:
    void writeFile(const QString& path) override;
};

#endif // INPUTMETHODEDITOR_H
