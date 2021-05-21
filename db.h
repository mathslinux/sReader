#ifndef DB_H
#define DB_H

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QList>

#define DB_VERSION 2

typedef struct Feed {
    qint32 id;
    QString title;
    QString description;
    QString link;
} Feed;

typedef struct News {
    qint32 id;
    qint32 feed_id;
    QString guid;
    QString title;
    QString link;
    QString pubDate;
    QString content;
    int read;
} News;

class DB
{
public:
    DB(const QString &name);
    bool connect();
    void disconnect();
    bool create_feed(Feed &feed);
    bool delete_feed(int feed_id);
    QList<Feed> get_all_feed();
    bool update_feed(Feed &feed);
    int unread_news_in_feed(int feed_id);
    bool create_news(News &news);
    QList<News> get_news_by_feed_id(int feed_id);
    bool news_has_guid(QString guid);
    bool news_has_link(QString link);
    News get_news_by_id(int news_id);
    bool update_news_read_status(int news_id, int read);

private:
    QSqlDatabase database;
    QString name;
    void (DB::*update_func[DB_VERSION])(void);

    void create_table();
    void check_update();
    void update_v1_to_v2();
#if 0
    void update_v2_to_v3();
    void update_v3_to_v4();
    void update_v4_to_v5();
#endif
};

#endif // DB_H
