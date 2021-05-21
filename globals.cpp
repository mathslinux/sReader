#include "globals.h"
#include <QDir>

QString Globals::app_path = "";
QString Globals::config_file_path = "";
QString Globals::database_path = "";

void Globals::init()
{
    QStringList list_;
    QDir dir;

    app_path = QString("%1/%2").arg(QDir::homePath()).arg(".sreader");
    if (!dir.exists(app_path)) {
        dir.mkpath(app_path);
    }

    // TODO: windows support
    config_file_path = QString("%1/%2").arg(app_path).arg("sreader.conf");
    database_path = QString("%1/%2").arg(app_path).arg("sreader.db");
}

QString Globals::get_app_path()
{
    return app_path;
}

QString Globals::get_config_file_path()
{
    return config_file_path;
}

QString Globals::get_database_path()
{
    return database_path;
}
