#ifndef CMDLISTWIDGET_H
#define CMDLISTWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QList>
class CMDListWidget : public QListWidget
{
    Q_OBJECT
public:
    using IconStrList = QList<QPair<QIcon, QString>>;

    CMDListWidget(QWidget* parent = nullptr);
    void addIconItems(const IconStrList& list);
    void adjustSizeEx(void);
    void selectNext(void);
    void selectPre(void);
    QString selectedText(void);

private:
    static constexpr int Item_H = 25;

signals:
    void itemActivedEx(QListWidgetItem* item); //双击或回车

    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // CMDLISTWIDGET_H
