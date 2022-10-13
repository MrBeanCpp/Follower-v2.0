#include "cacheiconprovider.h"
#include <QUrl>
#include <QDebug>
CacheIconProvider::CacheIconProvider()
{
}

QIcon CacheIconProvider::getUrlIcon(const QString& path)
{
    QIcon icon;
    if (!path.isEmpty()) {
        QString scheme = QUrl(path).scheme();
        if (scheme.startsWith("http")) //特殊处理网址
            icon = QIcon(":/images/web.png");
        else
            icon = QFileIconProvider::icon(QFileInfo(path));
    }
    return icon;
}

void CacheIconProvider::addCache(const QString& path)
{
    iconCache[path] = getUrlIcon(path);
}

QIcon CacheIconProvider::icon(const QString& path) //优先从缓存中读取
{
    QIcon icon = iconCache.value(path);
    if (icon.isNull() && path != "") {
        iconCache[path] = icon = getUrlIcon(path);
        qDebug() << "Not Hit IconCache:" << path;
    }
    return icon;
}
