#include "request.h"
#include <QtDebug>
#include <QUrl>
#include <QTime>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <stdlib.h>
#include <time.h>
#include <QTimer>
#include <QCryptographicHash> //md5加密的库
#include <QEventLoop>
Request::Request(QObject* parent)
    : QObject(parent)
{
    srand(time(NULL));
    Manager = new QNetworkAccessManager(this);
}

void Request::translate(const QString& text, std::function<void(const QString&)> todo, int timeout, const QString& from, const QString& to) //内部微循环 阻塞
{
    static const QString ID = "20200920000569944";
    static const QString Key = "pcbrOo74ISpnZA9M4KX3";
    QString salt = QString::number(rand());
    QString sign = ID + text + salt + Key;
    QString MD5 = QCryptographicHash::hash(sign.toUtf8(), QCryptographicHash::Md5).toHex(); //生成md5加密文件
    QString url = QString("http://api.fanyi.baidu.com/api/trans/vip/translate?"
                          "q=%1&from=%2&to=%3&appid=%4&salt=%5&sign=%6")
                      .arg(text)
                      .arg(from)
                      .arg(to)
                      .arg(ID)
                      .arg(salt)
                      .arg(MD5);
    QNetworkReply* reply = Manager->get(QNetworkRequest(QUrl(url))); //发送上传

    static auto decodeTransReply = [](QNetworkReply* reply) -> QString {
        //{"from":"en","to":"zh","trans_result":[{"src":"apple","dst":"\u82f9\u679c"}]}
        QString res;
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object(); //json转码；
        if (json.contains("error_code")) //传过来错误
            res = json.value("error_code").toString();
        else
            res = json.value("trans_result").toArray().at(0).toObject().value("dst").toString();
        reply->deleteLater();
        return res;
    };

    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeout);

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (timer.isActive()) { //normal
        timer.stop();
        todo(decodeTransReply(reply));
    } else { //time out
        reply->abort(); //终止reply，close() & 同时触发finished信号，可以用reply->isOpen()判断是否正常结束(可读)
        reply->deleteLater();
        todo("");
    }
}
