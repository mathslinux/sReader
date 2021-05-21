#include "db.h"
#include <QStringList>
#include <QDebug>

#define CREATE_SREADER_TABLE "CREATE TABLE sreader("\
    "id integer primary key, "\
    "key varchar, "\
    "value varchar"\
    ")"

#define CREATE_FEEDS_TABLE "CREATE TABLE feeds("\
    "id integer primary key, "\
    "title varchar, "\
    "description varchar, "\
    "link varchar"\
    ")"

#define CREATE_NEWS_TABLE_v1 "CREATE TABLE news("\
    "id integer primary key, "\
    "feed_id integer ,"\
    "guid varchar, "\
    "title varchar, "\
    "link varchar, "\
    "pubDate varchar, "\
    "content varchar"\
    ")"

#define CREATE_NEWS_TABLE "CREATE TABLE news("\
    "id integer primary key, "\
    "feed_id integer ,"\
    "guid varchar, "\
    "title varchar, "\
    "link varchar, "\
    "pubDate varchar, "\
    "content varchar, "\
    "read integer DEFAULT 0"\
    ")"


DB::DB(const QString &name) : name(name)
{
    update_func[1] = &DB::update_v1_to_v2;
#if 0
    update_func[2] = &DB::update_v2_to_v3;
    update_func[3] = &DB::update_v3_to_v4;
    update_func[4] = &DB::update_v4_to_v5;
#endif
}

bool DB::connect()
{
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(name);
    if (database.open()) {
        if (!database.tables().contains("sreader")) {
            create_table();
        } else {
            check_update();
        }
        return true;
    } else {
        return false;
    }
}

void DB::create_table()
{
    QSqlQuery sqlQuery;

    sqlQuery.prepare(CREATE_SREADER_TABLE);
    if (!sqlQuery.exec()) {
        qCritical("Create sreader table error");
        exit(1);
    }
    sqlQuery.prepare(QString("INSERT INTO sreader (key, value) VALUES('version', '%1')").arg(DB_VERSION));
    if (!sqlQuery.exec()) {
        qCritical("Init sreader table error");
        exit(1);
    }

    sqlQuery.prepare(CREATE_FEEDS_TABLE);
    if (!sqlQuery.exec()) {
        qCritical("Create feeds table error");
        exit(1);
    }

    sqlQuery.prepare(CREATE_NEWS_TABLE);
    if (!sqlQuery.exec()) {
        qCritical("Create news table error");
        exit(1);
    }
}

// 检查版本号，如果不符合，升级
void DB::check_update()
{
    int version;
    QSqlQuery q;
    q.exec("SELECT value FROM sreader WHERE key='version'");
    if (!q.first()) {
        qCritical("Check database version error");
        exit(1);
    }
    version = q.value(0).toInt();
    if (version < DB_VERSION) {
        qDebug() << "Update database from version " << version << " to " << DB_VERSION;
        for (int i = version; i < DB_VERSION; i++) {
            (this->*update_func[i])();
        }
    }
}

void DB::update_v1_to_v2()
{
    qDebug() << __FUNCTION__;

    QSqlQuery q;
    q.exec(QString("UPDATE sreader SET value='%1' WHERE key='version'").arg(DB_VERSION));
    q.exec(QString("ALTER TABLE news ADD COLUMN read integer DEFAULT 0"));
}

#if 0
void DB::update_v2_to_v3()
{
    qDebug() << __FUNCTION__;
}

void DB::update_v3_to_v4()
{
    qDebug() << __FUNCTION__;
}

void DB::update_v4_to_v5()
{
    qDebug() << __FUNCTION__;
}
#endif

void DB::disconnect()
{
    database.close();
}

bool DB::create_feed(Feed &feed)
{
    QSqlQuery query;
    query.prepare("INSERT INTO feeds (title, description, link) VALUES(?, ?, ?)");
    query.bindValue(0, feed.title);
    query.bindValue(1, feed.description);
    query.bindValue(2, feed.link);
    return query.exec();
}

bool DB::delete_feed(int feed_id)
{
    QSqlQuery q;
    return q.exec(QString("DELETE FROM feeds WHERE id=%1").arg(feed_id));
}

QList<Feed> DB::get_all_feed()
{
    QSqlQuery q;
    QList<Feed> feed_list;
    q.exec("SELECT id, title, description, link FROM feeds");
    while (q.next()) {
        Feed feed;
        feed.id = q.value(0).toInt();
        feed.title = q.value(1).toString();
        feed.description = q.value(2).toString();
        feed.link = q.value(3).toString();
        feed_list.append(feed);
    }
    return feed_list;
}

bool DB::update_feed(Feed &feed)
{
    QSqlQuery q;
    QString cmd = QString("UPDATE feeds SET title='%1', description='%2', link='%3' WHERE id=%4")\
            .arg(feed.title).arg(feed.description).arg(feed.link).arg(feed.id);

    return q.exec(cmd);

}

int DB::unread_news_in_feed(int feed_id)
{
    QSqlQuery q;
    q.exec(QString("SELECT count() FROM news WHERE feed_id=%1 AND read=0").arg(feed_id));
    if (q.first()) {
        return q.value(0).toInt();
    } else {
        return 0;
    }
}

bool DB::create_news(News &news)
{
    QSqlQuery q;
    q.prepare("INSERT INTO NEWS (feed_id, guid, title, link, pubDate, content) VALUES(?, ?, ?, ?, ?, ?)");
    q.bindValue(0, news.feed_id);
    q.bindValue(1, news.guid);
    q.bindValue(2, news.title);
    q.bindValue(3, news.link);
    q.bindValue(4, news.pubDate);
    q.bindValue(5, news.content);
    return q.exec();
}

QList<News> DB::get_news_by_feed_id(int feed_id)
{
    QSqlQuery q;
    QList<News> news_list;
    q.exec(QString("SELECT id, feed_id, guid, title, link, pubDate, content, read "\
        "FROM news WHERE feed_id=%1 ORDER BY id DESC").arg(feed_id));
    while (q.next()) {
        News news;
        news.id = q.value(0).toInt();
        news.feed_id = q.value(1).toInt();
        news.guid = q.value(2).toString();
        news.title = q.value(3).toString();
        news.link = q.value(4).toString();
        news.pubDate = q.value(5).toString();
        news.content = q.value(6).toString();
        news.read = q.value(7).toInt();
        news_list.append(news);
    }
    return news_list;
}

bool DB::news_has_guid(QString guid)
{
    QSqlQuery q;
    q.exec(QString("SELECT id FROM news WHERE guid='%1'").arg(guid));
    if (q.first()) {
        return true;
    }
    return false;
}

bool DB::news_has_link(QString link)
{
    QSqlQuery q;
    q.exec(QString("SELECT id FROM news WHERE link='%1'").arg(link));
    if (q.first()) {
        return true;
    }
    return false;
}

// TODO: 使用指针, 失败返回 NULL
News DB::get_news_by_id(int news_id)
{
    QSqlQuery q;
    News news;
    QString cmd = QString("SELECT id, feed_id, guid, title, link, pubDate, content, read FROM NEWS WHERE id=%1").\
            arg(news_id);
    q.exec(cmd);
    if (q.first()) {
        news.id = q.value(0).toInt();
        news.feed_id = q.value(1).toInt();
        news.guid = q.value(2).toString();
        news.title = q.value(3).toString();
        news.link = q.value(4).toString();
        news.pubDate = q.value(5).toString();
        news.content = q.value(6).toString();
        news.read = q.value(7).toInt();
    }
    return news;
}

bool DB::update_news_read_status(int news_id, int read)
{
    QSqlQuery q;

    QString cmd = QString("UPDATE news SET read=%1 WHERE id=%4").arg(read).arg(news_id);

    return q.exec(cmd);
}
