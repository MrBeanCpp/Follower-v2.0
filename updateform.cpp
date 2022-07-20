#include "updateform.h"
#include "ui_updateform.h"
#include <QFile>
#include <QProcess>
#include <QDesktopServices>
#include <QTimer>
#include <QTextStream>
#include <QDir>
UpdateForm::UpdateForm(QWidget* parent)
    : QDialog(parent), ui(new Ui::UpdateForm)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setWindowTitle("UpdateForm");
    qDebug() << QSslSocket::sslLibraryBuildVersionString() << QSslSocket::supportsSsl(); //需要安装openssl

    resize(DPI(size()));
    manager = new QNetworkAccessManager(this);

    connect(ui->btn_auto, &QPushButton::clicked, [=]() {
        updateLatestGiteeRelease(releaseUrl, "download.zip");
    });
    connect(ui->btn_manual, &QPushButton::clicked, [=]() {
        QDesktopServices::openUrl(QUrl(releaseUrl));
    });
}

UpdateForm::~UpdateForm()
{
    delete ui;
}

QUrl UpdateForm::getRedirectTarget(QNetworkReply* reply)
{
    return reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
}

void UpdateForm::download(const QUrl& url, const QString& path, std::function<void(bool)> todo)
{
    if (url.isEmpty()) return;

    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);
    static bool isSuccess;
    isSuccess = false;

    QFile::remove(path);
    ui->progressBar->setValue(0);

    connect(reply, &QNetworkReply::readyRead, [=]() {
        QUrl reTarget = getRedirectTarget(reply);
        if (reTarget.isValid()) return;

        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append))
            file.write(reply->readAll());
    });

    connect(reply, &QNetworkReply::finished, [=]() {
        qDebug() << "download finished";

        QUrl reTarget = getRedirectTarget(reply); //重定向
        reply->deleteLater();
        if (reTarget.isValid()) {
            qDebug() << reTarget;
            download(reTarget, path, todo);
        } else
            todo(isSuccess);
    });

    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 received, qint64 total) {
        isSuccess = (received == total);
        qDebug() << 1.0 * received / total * 100 << "%";
        ui->progressBar->setValue(1.0 * received / total * 100);
    });

    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [=](QNetworkReply::NetworkError code) {
        qCritical() << code;
    });
}

void UpdateForm::getDownloadUrl(const QString& htmlUrl, std::function<void(QMap<QString, QString>)> todo)
{
    QNetworkRequest request { QUrl(htmlUrl) }; //{}否则被误认为函数声明
    QNetworkReply* reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, [=]() {
        qDebug() << "html finished";
        reply->deleteLater();

        QString html(reply->readAll());
        QMap<QString, QString> res;
        if (html.isEmpty()) {
            todo(res);
        } else {
            res = parseHtml(html);
            todo(res);
        }
    });
}

QMap<QString, QString> UpdateForm::parseHtml(const QString& html)
{
    QMap<QString, QString> res;

    QRegExp reg("<div class='ui celled list releases-download-list'>\\n<div class='item'>\\n<a href=\"(.*)\"");
    reg.setMinimal(true);
    reg.indexIn(html);
    res["url"] = "https://gitee.com" + reg.cap(1);

    reg.setPattern("<a class=\"download-archive\" data-ref=\"(.*)\"");
    reg.indexIn(html);
    res["version"] = reg.cap(1);

    reg.setPattern("<div class='markdown-body'>\\n<p>(.*)</p>");
    reg.indexIn(html);
    res["desc"] = reg.cap(1).replace(QString("<br>&#x000A;"), QChar('\n'));

    return res;
}

void UpdateForm::updateLatestGiteeRelease(const QString& releaseUrl, const QString& filePath)
{
    ui->btn_auto->setEnabled(false);
    getDownloadUrl(releaseUrl, [=](QMap<QString, QString> res) {
        QString url = res["url"];
        QString version = res["version"];
        qDebug() << url;
        qDebug() << version;

        if (url.isEmpty()) {
            ui->label->setText("#Network Error");
        } else
            download(QUrl(url), filePath, [=](bool isSuccess) {
                if (isSuccess) {
                    const QString batPath = "unzip.bat";
                    writeBat(batPath, filePath, qApp->applicationDirPath());
                    QDesktopServices::openUrl(QUrl::fromLocalFile(batPath));
                    qApp->quit();
                } else {
                    qDebug() << "download Failed";
                    ui->label->setText("!Error: Download Failed");
                }
                ui->btn_auto->setEnabled(true);
            });
    });
}

void UpdateForm::writeBat(const QString& batPath, const QString& zipPath, const QString& folder)
{
    QFile file(batPath);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream text(&file);
        text << "@timeout /t 2 /NOBREAK" << '\n';
        text << "@cd /d %~dp0" << '\n'; //切换到bat目录，否则为qt的exe目录
        text << "@echo ##Unziping and replacing the files, please wait......\n";
        text << QString("7z x \"%1\" -o\"%2\" -aoa\n").arg(QDir::toNativeSeparators(zipPath)).arg(QDir::toNativeSeparators(folder));
        text << "@echo -------------------------------------------------------\n";
        text << "@echo ##Update SUCCESSFUL(Maybe)\n";
        text << "@echo -------------------------------------------------------\n";
        text << QString("@del \"%1\"\n").arg(zipPath);
        text << "@pause\n";
        text << "@start \"\" Follower.exe\n"; //start
        text << "@timeout /t 1 /NOBREAK" << '\n';
        text << "@exit";
    }
}

void UpdateForm::showEvent(QShowEvent* event)
{
    Q_UNUSED(event)
    getDownloadUrl(releaseUrl, [=](QMap<QString, QString> res) {
        if (res.empty())
            ui->label->setText("ERROR:empty result");
        else {
            ui->label->setText(QString("当前版本：%1\n最新版本：%2\n\n%3").arg(ver).arg(res["version"]).arg(res["desc"]));
            ui->btn_auto->setEnabled(ver != res["version"]);
        }
        ui->label->adjustSize();
    });
}
