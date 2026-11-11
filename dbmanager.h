#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include "models.h"

class DBManager : public QObject {
    Q_OBJECT

public:
    static DBManager& instance();

    bool openDatabase();

    // Операции с номерами
    QList<Room> getAllRooms();
    bool addRoom(const Room &room);
    bool updateRoom(const Room &room);
    bool deleteRoom(int roomId, QString &errorMessage);
    QList<Room> getAvailableRoomsForDates(const QDate &checkIn, const QDate &checkOut, int excludeBookingId = 0);

    // Операции с услугами
    QList<Service> getAllServices();
    bool addService(const Service &service);
    bool updateService(const Service &service);
    bool deleteService(int serviceId, QString &errorMessage);

    // Операции с гостями
    QList<Guest> getAllGuests();
    int addOrGetGuest(const Guest &guest);

    // Операции с бронированиями
    QList<Booking> getAllBookings();
    Booking getBookingById(int bookingId);
    bool addBooking(Booking &booking);
    bool updateBooking(const Booking &booking);
    bool cancelBooking(int bookingId);
    QList<Booking> getGuestHistory(int guestId);

    // Автоматическая актуализация статусов
    void autoCompleteFinishedBookings();

private:
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager() override;

    DBManager(const DBManager&) = delete;
    DBManager& operator=(const DBManager&) = delete;

    bool createTables();
    void seedInitialData();

    QSqlDatabase m_db;
};

#endif // DBMANAGER_H