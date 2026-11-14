#ifndef DBMANAGER_H
#define DBMANAGER_H

#include "models.h"
#include <QSqlDatabase>
#include <QList>
#include <QString>
#include <QDate>

class DBManager {
public:
    explicit DBManager(const QString &dbName = "hotel_management.db");
    ~DBManager();

    bool openDatabase();
    bool createTables();

    QList<Room> getAllRooms();
    bool addRoom(const Room &room);
    bool updateRoom(const Room &room);
    bool deleteRoom(int roomId);
    bool deleteRoom(int roomId, QString &errorMsg);
    bool isRoomAvailable(int roomId, const QDate &checkIn, const QDate &checkOut, int excludeBookingId = -1);

    QList<Service> getAllServices();
    bool addService(const Service &service);
    bool updateService(const Service &service);
    bool deleteService(int serviceId);
    bool deleteService(int serviceId, QString &errorMsg);

    QList<Guest> getAllGuests();
    int addOrGetGuest(const Guest &guest);
    QList<Booking> getGuestHistory(int guestId);

    QList<Booking> getAllBookings();
    QList<int> getBookingServices(int bookingId);
    bool addBooking(const Booking &booking);
    bool updateBooking(const Booking &booking);
    bool updateBookingStatus(int bookingId, const QString &newStatus);
    bool cancelBooking(int bookingId);
    bool deleteBooking(int bookingId);
    void autoCompleteFinishedBookings();

private:
    QSqlDatabase m_db;
    QString m_dbName;
};

#endif