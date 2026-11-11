#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void openBookingDialog();
    void editSelectedBooking();
    void cancelSelectedBooking();

    void addRoomDialog();
    void editRoomDialog();
    void deleteSelectedRoom();

    void addServiceDialog();
    void editServiceDialog();
    void deleteSelectedService();

    void showGuestHistory();

private:
    void setupUI();
    void loadRooms();
    void loadBookings();
    void loadGuests();
    void loadServices();

    QTabWidget *m_tabWidget;

    QTableWidget *m_roomsTable;
    QTableWidget *m_bookingsTable;
    QTableWidget *m_guestsTable;
    QTableWidget *m_servicesTable;

    QPushButton *m_addRoomBtn;
    QPushButton *m_editRoomBtn;
    QPushButton *m_deleteRoomBtn;

    QPushButton *m_newBookingBtn;
    QPushButton *m_editBookingBtn;
    QPushButton *m_cancelBookingBtn;

    QPushButton *m_guestHistoryBtn;

    QPushButton *m_addServiceBtn;
    QPushButton *m_editServiceBtn;
    QPushButton *m_deleteServiceBtn;
};

#endif // MAINWINDOW_H