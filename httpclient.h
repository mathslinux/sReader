#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QUrl>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QList>

class HTTPClient : public QObject
{
    Q_OBJECT
public:
    HTTPClient();
    ~HTTPClient();

    void load(QString &url);
    void load(QString &url, int feed_id);
    QByteArray get_data(QNetworkReply *reply);

signals:
    // TODO: error signal
    void readyRead(QNetworkReply *reply, int feed_id);

private:
    QList<int> feed_ids;
    QList<QUrl> feed_urls;
    QNetworkAccessManager *manager;
    QNetworkRequest request;

private slots:
    void finished(QNetworkReply *reply);
    void error(QNetworkReply::NetworkError code);
};

#endif // HTTPCLIENT_H
