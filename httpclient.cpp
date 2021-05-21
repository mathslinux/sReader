#include "httpclient.h"
#include <QMessageBox>
#include <QSqlQuery>  // TODO: delete

HTTPClient::HTTPClient()
{
    manager = new QNetworkAccessManager();
    connect(manager, &QNetworkAccessManager::finished, this, &HTTPClient::finished);
}

HTTPClient::~HTTPClient()
{
    delete manager;
}

void HTTPClient::load(QString &url, int feed_id)
{
    request.setUrl(QUrl(url));
    manager->get(request);
    // TODO: error slot
    // connect(reply, &QNetworkReply::errorOccurred, this, &HTTPClient::error);
    feed_ids.append(feed_id);
    feed_urls.append(url);
}

QByteArray HTTPClient::get_data(QNetworkReply *reply)
{
    // TODO: check return code from http server
    return reply->readAll();
}

void HTTPClient::finished(QNetworkReply *reply)
{
    // TODO: 判断 index
    int index = feed_urls.indexOf(reply->url());
    feed_urls.removeAt(index);
    int feed_id = feed_ids.takeAt(index);

    if (reply->error() != QNetworkReply::NoError) {
        // TODO: 发射error信号，并且移到外部处理 UI
        QMessageBox msgBox;
        msgBox.setText(QString("Request %1 error: %2").arg(reply->url().toString()).arg(reply->errorString()));
        msgBox.exec();
        reply->deleteLater();

        QSqlQuery q;
        q.exec(QString("DELETE FROM feeds WHERE id=%1").arg(feed_id));
        return ;
    }
    emit readyRead(reply, feed_id);
}

void HTTPClient::error(QNetworkReply::NetworkError code)
{
    // TODO
}
