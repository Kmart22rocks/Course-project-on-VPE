#include <QApplication>
#include <QPalette>
#include <QPixmap>
#include <QPainter>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    QPixmap checkPx(14, 14);
    checkPx.fill(Qt::transparent);
    QPainter painter(&checkPx);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#f0f2f5"), 2.2));
    painter.drawLine(3, 7, 6, 10);
    painter.drawLine(6, 10, 11, 4);
    painter.end();
    checkPx.save("check.png");

    QColor bg = app.palette().color(QPalette::Window);
    QColor mainWinColor = app.palette().color(QPalette::Button).lighter(126);

    app.setStyleSheet(QString(
        "QDialog { background-color: %4; }"
        "QPushButton { background-color: %1; border: 1px solid %2; border-radius: 4px; padding: 5px 12px; }"
        "QPushButton:hover { background-color: %3; }"
        "QAbstractItemView { selection-background-color: #96c579; selection-color: #1c221e; }"
        "QAbstractItemView::item:selected { background-color: #96c579; color: #1c221e; }"
        "QCheckBox::indicator, QAbstractItemView::indicator { width: 14px; height: 14px; border: 1px solid #f0f2f5; border-radius: 3px; background: transparent; }"
        "QCheckBox::indicator:checked, QAbstractItemView::indicator:checked { image: url(check.png); }"
    ).arg(bg.darker(115).name(),
          bg.lighter(115).name(),
          bg.lighter(130).name(),
          mainWinColor.name()));

    MainWindow w;
    w.show();
    return app.exec();
}