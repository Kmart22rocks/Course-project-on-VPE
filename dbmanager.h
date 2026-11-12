#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include "models.h"

class DBManager : public QObject {
    Q_OBJECT
public:
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager() override;

    bool openDatabase(const QString &path = "hotel_management.db");

    QList<Room> getAllRooms();
    bool addRoom(const Room &room);
    bool updateRoom(const Room &room);
    bool deleteRoom(int roomId, QString &errorMsg);
    QList<Room> getAvailableRoomsForDates(const QDate &checkIn, const QDate &checkOut, int excludeBookingId = 0);

    QList<Service> getAllServices();
    bool addService(const Service &service);
    bool updateService(const Service &service);
    bool deleteService(int serviceId, QString &errorMsg);

    QList<Guest> getAllGuests();
    int addOrGetGuest(const Guest &guest);

    QList<Booking> getAllBookings();
    Booking getBookingById(int bookingId);
    bool addBooking(Booking &booking);
    bool updateBooking(const Booking &booking);
    bool cancelBooking(int bookingId);
    QList<Booking> getGuestHistory(int guestId);

    void autoCompleteFinishedBookings();

private:
    bool createTables();
    void seedInitialData();

    QSqlDatabase m_db;
};

#endif // DBMANAGER_H