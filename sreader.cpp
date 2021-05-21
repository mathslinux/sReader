#include "sreader.h"
#include "utils.h"
#include "globals.h"
#include "ui_sreader.h"
#include <QXmlStreamReader>
#include <QDomDocument>
#include <QUrl>
#include <QMessageBox>
#include <QLabel>

// TODO: move all database operation to db.cpp

SReader::SReader(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SReader)
{
    ui->setupUi(this);

    init_db();
    init_splitter();
    init_http_client();
    init_feed_add();
    init_feed_tree();
    init_news();
    resize(800, 600);
}

SReader::~SReader()
{
    delete ui;
    // TODO: delete httpclient and so on.
    db->disconnect();
}

void SReader::init_feed_add()
{
    connect(ui->feed_add_button, &QPushButton::clicked, this, &SReader::feed_add_button_clicked);
}

void SReader::init_feed_tree()
{
    // FIXME: memory leak?
    QList<Feed> feed_list;

    ui->feed_tree->setHeaderHidden(true);
    ui->feed_tree->setColumnCount(3);
    ui->feed_tree->setRootIsDecorated(false); //去掉虚线边框
    ui->feed_tree->setFrameStyle(QFrame::NoFrame); //去掉边框
    ui->feed_tree->setStyleSheet("QTreeView::branch {image:none;}"); //去掉子节点的虚框


    connect(ui->feed_tree, &QTreeWidget::itemClicked, this, &SReader::feed_clicked);
    //connect(ui->feed_tree, &QTreeWidget::clicked)
    ui->feed_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->feed_tree, &QTreeWidget::customContextMenuRequested, this, &SReader::feed_menu_requested);

    // just display feed title
    ui->feed_tree->hideColumn(1);

    feed_list = db->get_all_feed();
    for (QList<Feed>::iterator it = feed_list.begin(); it != feed_list.end(); it++) {
         QTreeWidgetItem *item = new QTreeWidgetItem(ui->feed_tree);
         item->setText(0, it->title);
         item->setText(1, QString::number(it->id));

         int unread = db->unread_news_in_feed(it->id);
         if (unread > 0) {
             QLabel *unread_label = new QLabel(QString::number(unread));
             unread_label->setStyleSheet("min-width: 16px; min-height: 16px;max-width:30px; "\
                    "max-height: 30px;border-radius: 8px;  border:1px solid black;background:grey");
             ui->feed_tree->setItemWidget(item, 2, unread_label);
             item->setTextAlignment(2, Qt::AlignRight);
         }

         ui->feed_tree->addTopLevelItem(item);
    }
}

void SReader::feed_add_button_clicked()
{
    // TODO: 统一的 url, 比如 "/" 结尾等等。
    QString feed_url = ui->feed_edit->text().simplified();
    QUrl qurl(feed_url);
    QSqlQuery q;

    if (QUrl(feed_url).scheme().isEmpty()) {
        qurl.setUrl(QString("http://%1").arg(feed_url));
        feed_url = qurl.toString();
        ui->feed_edit->setText(feed_url);
    }

    q.exec(QString("SELECT id FROM feeds where link='%1'").arg(feed_url));
    if (q.first()) {
        // TODO: 封装
        QMessageBox msgBox;
        msgBox.setText(QString("Error: duplicate link %1").arg(feed_url));
        msgBox.exec();
        q.finish();
        return ;
    }
    q.finish();

    Feed feed;
    feed.link = feed_url;
    db->create_feed(feed);

    q.exec(QString("SELECT id FROM feeds where link='%1'").arg(feed_url));
    q.first();
    http_client->load(feed_url, q.value(0).toInt());
    q.finish();
}

void SReader::feed_clicked(QTreeWidgetItem *item, int)
{
    int feed_id = item->text(1).toInt();
    this->feed_show_news(feed_id);
}

void SReader::feed_menu_requested(const QPoint &pos)
{
    QMenu *menu = new QMenu();

    QAction *action_reflush = new QAction("刷新");
    connect(action_reflush, &QAction::triggered, this, &SReader::feed_reflush_triggered);
    action_reflush->setData(pos);
    menu->addAction(action_reflush);

    QAction *action_delete = new QAction("删除");
    connect(action_delete, &QAction::triggered, this, &SReader::feed_delete_triggered);
    action_delete->setData(pos);
    menu->addAction(action_delete);

    menu->exec(QCursor::pos());
}

void SReader::feed_reflush_triggered(bool)
{
    QAction *action = qobject_cast<QAction *>(sender());
    QTreeWidgetItem *item = ui->feed_tree->itemAt(action->data().toPoint());
    int feed_id = item->text(1).toInt();
    QSqlQuery q;
    q.exec(QString("SELECT link FROM feeds where id=%1").arg(feed_id));
    q.first();
    QString link = q.value(0).toString();
    http_client->load(link, feed_id);
}

void SReader::feed_delete_triggered(bool)
{
    QAction *action = qobject_cast<QAction *>(sender());
    QTreeWidgetItem *item = ui->feed_tree->itemAt(action->data().toPoint());
    int feed_id = item->text(1).toInt();

    QSqlQuery q;
    q.exec(QString("DELETE FROM feeds where id=%1").arg(feed_id));
    q.exec(QString("DELETE FROM news where feed_id=%1").arg(feed_id));

    delete item;
}

void SReader::init_news()
{
    // No highlihgt for selected item.
    //ui->news->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //ui->news->setFocusPolicy(Qt::NoFocus);
    //ui->news->setSelectionMode(QAbstractItemView::NoSelection);

    // Hide id and read column.
    ui->news->hideColumn(2);
    ui->news->hideColumn(3);
    // TODO: 重写 resize 事件达到按比例伸缩的效果.
    ui->news->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 设置选择为行选择
    ui->news->setSelectionBehavior(QTableWidget::SelectRows);

    // disable editor
    ui->news->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 设置未读news为粗体。
    QFont font = ui->news->font();
    font.setBold(true);
    ui->news->setFont(font);

    connect(ui->news, &QTableWidget::cellClicked, this, &SReader::news_clicked);
    connect(ui->news, &QTableWidget::currentCellChanged, this, &SReader::news_changed);
}

void SReader::feed_show_news(int feed_id)
{
    QString pubDate;
    QList<News> news_list = db->get_news_by_feed_id(feed_id);
    ui->news->setRowCount(news_list.size());
    QFont font = ui->news->font();

    for (int i = 0; i < news_list.size(); i++) {
        if (news_list.at(i).read) {
            font.setBold(false);
        } else {
            font.setBold(true);
        }
        QTableWidgetItem *title_item = new QTableWidgetItem(news_list.at(i).title);
        title_item->setFont(font);
        ui->news->setItem(i, 0, title_item);
        pubDate = news_list.at(i).pubDate; // TODO: 优化时间显示
        QTableWidgetItem *date_item = new QTableWidgetItem(pubDate);
        date_item->setFont(font);
        ui->news->setItem(i, 1, date_item);
        ui->news->setItem(i, 2, new QTableWidgetItem(QString::number(news_list.at(i).id)));
        ui->news->setItem(i, 3, new QTableWidgetItem(QString::number(news_list.at(i).read)));
    }
}

void SReader::news_show(int row)
{
    const char *temp = "<h1>%1</h1><p>%2</p>";
    News news;
    int news_id;
    int read;
    QFont font;

    news_id = ui->news->item(row, 2)->text().toInt();
    news = db->get_news_by_id(news_id);

    read = ui->news->item(row, 3)->text().toInt();
    if (!read) {
        font = ui->news->font();
        font.setBold(false);
        ui->news->item(row, 0)->setFont(font);
        ui->news->item(row, 1)->setFont(font);
        ui->news->item(row, 3)->setText("1");
        db->update_news_read_status(news_id, 1);

        // TODO: 封装该方法
        QList<QTreeWidgetItem *> item_list = ui->feed_tree->findItems(
                    QString::number(news.feed_id), Qt::MatchFixedString, 1);
        if (item_list.size()) {
            QTreeWidgetItem *item = item_list.first();
            int unread = db->unread_news_in_feed(news.feed_id);
            if (unread > 0) {
                QLabel *unread_label = new QLabel(QString::number(unread));
                unread_label->setStyleSheet("min-width: 16px; min-height: 16px;max-width:30px; "\
                   "max-height: 30px;border-radius: 8px;  border:1px solid black;background:grey");
                ui->feed_tree->setItemWidget(item, 2, unread_label);
            } else {
                ui->feed_tree->removeItemWidget(item, 2);
            }
        }
    }

    ui->web_title->setHtml(QString(temp).arg(news.title).arg(news.pubDate));
    ui->web_view->setHtml(news.content);
    ui->web_view->show();
}

void SReader::news_clicked(int row, int column)
{
    this->news_show(row);
}

void SReader::news_changed(int row, int, int, int)
{
    this->news_show(row);
}

void SReader::process_feed_response(QNetworkReply *reply, int feed_id)
{
    QDomDocument doc;
    Feed feed;
    int news_size;

    feed.id = feed_id;
    feed.link = reply->url().toString();
    doc.setContent(reply->readAll());
    reply->deleteLater();

    QDomElement root = doc.documentElement();
    QDomNode channel = root.namedItem("channel");
    feed.title = channel.namedItem("title").toElement().text().simplified();
    feed.description = channel.namedItem("description").toElement().text().simplified();

    db->update_feed(feed);

    // parse news from item tag
    QDomNodeList news_list = root.elementsByTagName("item");
    news_size = news_list.size();
    for (int i = 0; i < news_size; i++) {
        News news;
        QString description;
        QDomNode node = news_list.at(news_size - 1 - i);  // 按时间先后插入数据库。
        news.feed_id = feed_id;
        news.guid = node.namedItem("guid").toElement().text().simplified();
        if (!news.guid.isEmpty()) {
            // TODO: 优化: 预读所有 guid 和 link
            if (db->news_has_guid(news.guid)) {
                continue;
            }
        }
        news.title = node.namedItem("title").toElement().text().simplified();
        news.link = node.namedItem("link").toElement().text().simplified();
        if (!news.link.isEmpty()) {
            if (db->news_has_link(news.link)) {
                continue;
            }
        }
        news.pubDate = node.namedItem("pubDate").toElement().text().simplified();
        news.pubDate = parse_datestr(news.pubDate);
        news.content = node.namedItem("content").toElement().text();
        description = node.namedItem("description").toElement().text();
        if (news.content.isEmpty()) {
            news.content = node.namedItem("content:encoded").toElement().text();
        }
        if (news.content.isEmpty()) {
            news.content = description;
        }
        db->create_news(news);
    }

    // Add new feed to feed tree and show it
    // TODO: 封装
    QTreeWidgetItem *item;
    QList<QTreeWidgetItem *> item_list = ui->feed_tree->findItems(QString::number(feed.id), Qt::MatchFixedString, 1);
    if (!item_list.size()) {
        item = new QTreeWidgetItem(ui->feed_tree);
        item->setText(0, feed.title);
        item->setText(1, QString::number(feed.id));
        ui->feed_tree->addTopLevelItem(item);

        int unread = db->unread_news_in_feed(feed.id);
        if (unread > 0) {
            QLabel *unread_label = new QLabel(QString::number(unread));
            unread_label->setStyleSheet("min-width: 16px; min-height: 16px;max-width:30px; "\
                   "max-height: 30px;border-radius: 8px;  border:1px solid black;background:grey");
            ui->feed_tree->setItemWidget(item, 2, unread_label);
        }
    } else {
        // TODO: 设置未读labe。
    }

    // show news just added
    ui->news->clear();
    this->feed_show_news(feed_id);
}

void SReader::init_db()
{
    db = new DB(Globals::get_database_path());
    if (!db->connect()) {
        qCritical("load sreader.db error, exit");
        exit(1);
    }
}

#include <QFile>
void SReader::init_splitter()
{
    QList<int> sizes;
    // TODO： 配置文件
    sizes << 100 << 200 << 500;
    ui->splitter->setHandleWidth(1);
    ui->splitter->setSizes(sizes);
}

void SReader::init_http_client()
{
    http_client = new HTTPClient;
    connect(http_client, &HTTPClient::readyRead, this, &SReader::process_feed_response);
}
