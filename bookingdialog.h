#ifndef BOOKINGDIALOG_H
#define BOOKINGDIALOG_H

#include <QDialog>
#include <QDateEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include "dbmanager.h"
#include "currencyapiclient.h"
#include "models.h"

class BookingDialog : public QDialog {
    Q_OBJECT
public:
    explicit BookingDialog(DBManager *db,
                           CurrencyApiClient *apiClient,
                           QWidget *parent = nullptr,
                           int bookingIdToEdit = 0);
    Booking getBookingData() const;

private slots:
    void updateAvailableRooms();
    void calculateTotal();
    void validateAndAccept();

private:
    void setupUI();
    void loadData();
    QList<int> getSelectedServiceIds() const;
    double getSelectedRoomPrice() const;

    DBManager *m_db;
    CurrencyApiClient *m_apiClient;
    int m_bookingIdToEdit;
    Booking m_currentBooking;

    QLineEdit *m_nameEdit;
    QLineEdit *m_passportEdit;
    QLineEdit *m_phoneEdit;
    QDateEdit *m_checkInEdit;
    QDateEdit *m_checkOutEdit;
    QComboBox *m_roomCombo;
    QListWidget *m_servicesList;
    QLabel *m_totalLabel;

    QList<Room> m_availableRooms;
    QList<Service> m_allServices;
};

#endif // BOOKINGDIALOG_H