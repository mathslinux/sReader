#ifndef GLOBALS_H
#define GLOBALS_H

#include <QString>

class Globals
{
public:
    static void init();
    static QString get_app_path();
    static QString get_config_file_path();
    static QString get_database_path();

private:
    static QString app_path;
    static QString config_file_path;
    static QString database_path;
};

#endif // GLOBALS_H
