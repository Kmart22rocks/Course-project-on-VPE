#ifndef BOOKINGDIALOG_H
#define BOOKINGDIALOG_H

#include <QDialog>
#include "dbmanager.h"
#include "currencyapiclient.h"

class QLineEdit;
class QDateEdit;
class QComboBox;
class QListWidget;
class QLabel;
class QPushButton;

class BookingDialog : public QDialog {
    Q_OBJECT
public:
    explicit BookingDialog(DBManager *db,
                           CurrencyApiClient *apiClient = nullptr,
                           QWidget *parent = nullptr,
                           int editBookingId = -1);

    Guest getGuestData() const;
    Booking getBookingData() const;

private slots:
    void onExistingGuestSelected(int index);
    void onPassportEdited(const QString &passportText);
    void onShowGuestHistory();
    void onDatesChanged();
    void updateAvailableRooms();
    void calculateTotal();
    void validateAndAccept();

private:
    DBManager *m_db;
    CurrencyApiClient *m_apiClient;
    int m_editBookingId;
    int m_targetRoomId = -1;

    QList<Guest> m_guests;
    QList<Service> m_allServices;
    int m_selectedGuestId = -1;

    QComboBox *m_existingGuestCombo;
    QLineEdit *m_nameEdit;
    QLineEdit *m_passportEdit;
    QLineEdit *m_phoneEdit;
    QLabel *m_guestHistoryLabel;
    QPushButton *m_historyBtn;

    QDateEdit *m_checkInEdit;
    QDateEdit *m_checkOutEdit;
    QComboBox *m_roomCombo;
    QListWidget *m_servicesList;
    QLabel *m_totalLabel;

    void setupUi();
    void loadExistingGuests();
    void loadServices();
    void loadBookingForEdit();
    void updateGuestHistoryPreview(int guestId);
};

#endif