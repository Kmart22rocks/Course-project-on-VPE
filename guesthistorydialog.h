#ifndef GUESTHISTORYDIALOG_H
#define GUESTHISTORYDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include "dbmanager.h"
#include "models.h"

class GuestHistoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit GuestHistoryDialog(const Guest &guest, DBManager *db, QWidget *parent = nullptr);

private:
    void setupUI();
    void loadHistory();

    Guest m_guest;
    DBManager *m_db;
    QTableWidget *m_table;
    QLabel *m_summaryLabel;
};

#endif // GUESTHISTORYDIALOG_H
