#include "utils.h"
#include <QLocale>
#include <QDateTime>

// parse datetime string "Fri, 21 May 2021 08:17:00 +0800" which is from rss server
// to format "yyyy-MM-ddTHH:mm:ss"
// TODO: 解析失败使用系统locale，目前只考虑一种情况。
QString parse_datestr(QString &datestr)
{
  QString buf, temp;
  QLocale locale(QLocale::C);
  if (datestr.indexOf(",") != -1) {
    buf = datestr.remove(0, datestr.indexOf(",")+1).simplified();
  }
  temp = buf.left(20);
  return locale.toDateTime(temp, "dd MMM yyyy HH:mm:ss").toString("yyyy-MM-ddTHH:mm:ss");
}
