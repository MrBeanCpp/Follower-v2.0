#include "logfilehandler.h"
QtMessageHandler LogFileHandler::qtMsgLog = nullptr;
const QString LogFileHandler::dir = "/logs";
void LogFileHandler::startFileLogging(bool keepQtOutput)
{
    QString dirPath = QApplication::applicationDirPath() + dir;
    if (!QFileInfo::exists(dirPath))
        QDir().mkdir(dirPath);
    qtMsgLog = qInstallMessageHandler(outputMessage); //安装消息处理程序 return上一个处理函数
    if (!keepQtOutput)
        qtMsgLog = nullptr;
}

void LogFileHandler::stopFileLogging()
{
    qInstallMessageHandler(0);
}

void LogFileHandler::outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (qtMsgLog) qtMsgLog(type, context, msg); //输出到Qt Creator控制台
    static QMutex mutex;
    mutex.lock();
    QString text;
    switch (type) {
    case QtDebugMsg:
        text = QString("[Debug]");
        break;
    case QtWarningMsg:
        text = QString("[Warning]");
        break;
    case QtCriticalMsg:
        text = QString("[Critical]");
        break;
    case QtFatalMsg:
        text = QString("[Fatal]");
        break;
    default:
        text = QString("[Debug]");
    }

    // 设置输出信息格式
    QString strDateTime = QTime::currentTime().toString("HH:mm:ss.zzz");
    QString strMessage = QString("%1 %2 %3").arg(strDateTime, text, msg);
    // 输出信息至文件中（读写、追加形式）
    //必须确保目录存在（构造函数）
    QString filePath = QApplication::applicationDirPath() + dir + "/" + QDateTime::currentDateTime().toString("yyyy-MM-dd").append("-log.txt");
    QFile file(filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream stream(&file);
    stream << strMessage << "\n";
    file.flush();
    file.close();
    // 解锁
    mutex.unlock();
}
