#ifndef PATH_H
#define PATH_H

#include <QApplication>
#include <QString>
class Path { //static类，提供统一Path
public:
    Path() = delete; //不允许实例化
    static const QString dirName;

    static QString dirPath(void);
    static QString iniFile(void);
    static QString cmdList(void);
    static QString inputMethodList(void);
    static QString noteList(void);
};

#endif // PATH_H
