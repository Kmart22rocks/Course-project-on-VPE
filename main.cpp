#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"
#include "dbmanager.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    if (!DBManager::instance().openDatabase()) {
        QMessageBox::critical(nullptr, "Ошибка БД", "Не удалось подключиться к базе данных SQLite!");
        return -1;
    }

    MainWindow w;
    w.show();

    return a.exec();
}