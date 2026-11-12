#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include "dbmanager.h"
#include "currencyapiclient.h"
#include "networkservice.h"
#include "analyticswidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void loadRooms();
    void addRoom();
    void editRoom();
    void deleteRoom();

    void loadServices();
    void addService();
    void editService();
    void deleteService();

    void loadGuests();
    void viewGuestHistory();

    void loadBookings();
    void createBooking();
    void editBooking();
    void cancelBooking();

    void refreshNetworkAnalytics();

private:
    void setupUI();

    DBManager m_db;
    HotelTcpServer *m_tcpServer;
    HotelTcpClient *m_tcpClient;
    CurrencyApiClient *m_apiClient;

    QTabWidget *m_tabs;
    QTableWidget *m_roomsTable;
    QTableWidget *m_servicesTable;
    QTableWidget *m_guestsTable;
    QTableWidget *m_bookingsTable;
    AnalyticsWidget *m_analyticsWidget;
};

#endif // MAINWINDOW_H