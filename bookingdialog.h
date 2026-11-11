#ifndef BOOKINGDIALOG_H
#define BOOKINGDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include "models.h"

class BookingDialog : public QDialog {
    Q_OBJECT

public:
    explicit BookingDialog(QWidget *parent = nullptr, int bookingIdToEdit = 0);

private slots:
    void updateAvailableRooms();
    void calculateTotalCost();
    void onSave();

private:
    void setupUI();

    QLineEdit *m_nameEdit;
    QLineEdit *m_passportEdit;
    QLineEdit *m_phoneEdit;
    QDateEdit *m_checkInEdit;
    QDateEdit *m_checkOutEdit;
    QComboBox *m_roomCombo;
    QListWidget *m_servicesList;
    QLabel *m_totalCostLabel;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;

    QList<Room> m_availableRooms;
    QList<Service> m_allServices;
    int m_editBookingId;
    Booking m_existingBooking;
};

#endif // BOOKINGDIALOG_H