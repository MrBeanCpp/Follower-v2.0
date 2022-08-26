#include "path.h"
const QString Path::dirName = "/Core-Info";

QString Path::dirPath()
{
    return QApplication::applicationDirPath() + dirName;
}

QString Path::iniFile() //QApplication需要在main函数中对象初始化后才能使用，所以不能用static变量在编译阶段去确定
{
    return QApplication::applicationDirPath() + dirName + "/settings.ini";
}

QString Path::cmdList()
{
    return QApplication::applicationDirPath() + dirName + "/cmdList.txt";
}

QString Path::inputMethodList()
{
    return QApplication::applicationDirPath() + dirName + "/inputMethodList.txt";
}

QString Path::noteList()
{
    return QApplication::applicationDirPath() + dirName + "/noteList.txt";
}

QString Path::runTimesData()
{
    return QApplication::applicationDirPath() + dirName + "/runTimesMap.dat";
}
