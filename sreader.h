#ifndef SREADER_H
#define SREADER_H

#include <QMainWindow>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTableWidget>

#include "httpclient.h"
#include "db.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SReader; }
QT_END_NAMESPACE

class SReader : public QMainWindow
{
    Q_OBJECT

public:
    SReader(QWidget *parent = nullptr);
    ~SReader();

private slots:
    void feed_add_button_clicked();
    void feed_clicked(QTreeWidgetItem *item, int column);
    void feed_menu_requested(const QPoint &pos);
    void feed_reflush_triggered(bool checked = false);
    void feed_delete_triggered(bool checked = false);
    void news_clicked(int row, int column);
    void news_changed(int row, int column, int previousRow, int previousColumn);

private:
    QDockWidget *content_dock;
    HTTPClient *http_client;
    DB *db;

    void init_db();
    void init_splitter();
    void init_http_client();
    void init_feed_add();
    void init_feed_tree();
    void init_news();
    void feed_show_news(int feed_id);
    void news_show(int row);
    void process_feed_response(QNetworkReply *reply, int feed_id);

private:
    Ui::SReader *ui;
};
#endif // SREADER_H
