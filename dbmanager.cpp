#include "dbmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DBManager& DBManager::instance() {
    static DBManager instance;
    return instance;
}

DBManager::DBManager(QObject *parent) : QObject(parent) {}

DBManager::~DBManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DBManager::openDatabase() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("hotel_management.db");

    if (!m_db.open()) {
        qDebug() << "Ошибка открытия базы данных:" << m_db.lastError().text();
        return false;
    }

    if (!createTables()) {
        return false;
    }

    autoCompleteFinishedBookings();
    seedInitialData();
    return true;
}

bool DBManager::createTables() {
    QSqlQuery query;
    const QStringList tableQueries = {
        "CREATE TABLE IF NOT EXISTS rooms ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "room_number INTEGER UNIQUE NOT NULL, "
        "type TEXT NOT NULL, "
        "price_per_night REAL NOT NULL, "
        "status TEXT NOT NULL DEFAULT 'Свободен')",

        "CREATE TABLE IF NOT EXISTS guests ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "full_name TEXT NOT NULL, "
        "passport TEXT UNIQUE NOT NULL, "
        "phone TEXT NOT NULL)",

        "CREATE TABLE IF NOT EXISTS services ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "price REAL NOT NULL)",

        "CREATE TABLE IF NOT EXISTS bookings ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "guest_id INTEGER NOT NULL, "
        "room_id INTEGER NOT NULL, "
        "check_in TEXT NOT NULL, "
        "check_out TEXT NOT NULL, "
        "status TEXT NOT NULL, "
        "total_cost REAL NOT NULL, "
        "FOREIGN KEY(guest_id) REFERENCES guests(id), "
        "FOREIGN KEY(room_id) REFERENCES rooms(id))",

        "CREATE TABLE IF NOT EXISTS booking_services ("
        "booking_id INTEGER NOT NULL, "
        "service_id INTEGER NOT NULL, "
        "FOREIGN KEY(booking_id) REFERENCES bookings(id), "
        "FOREIGN KEY(service_id) REFERENCES services(id))"
    };

    for (const QString &sql : tableQueries) {
        if (!query.exec(sql)) {
            qDebug() << "Ошибка создания таблицы:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

void DBManager::autoCompleteFinishedBookings() {
    QSqlQuery query;
    query.prepare("UPDATE bookings SET status = 'Завершено' "
                  "WHERE status = 'Активно' AND check_out <= :today");
    query.bindValue(":today", QDate::currentDate().toString(Qt::ISODate));
    query.exec();
}

void DBManager::seedInitialData() {
    QSqlQuery query;

    query.exec("SELECT COUNT(*) FROM rooms");
    if (query.next() && query.value(0).toInt() == 0) {
        query.exec("INSERT INTO rooms (room_number, type, price_per_night) VALUES "
                   "(101, 'Стандарт', 2500.0), "
                   "(102, 'Люкс', 5000.0), "
                   "(201, 'Президентский', 12000.0), "
                   "(202, 'Семейный', 4000.0)");
    }

    query.exec("SELECT COUNT(*) FROM services");
    if (query.next() && query.value(0).toInt() == 0) {
        query.exec("INSERT INTO services (name, price) VALUES "
                   "('Завтрак \"Шведский стол\"', 600.0), "
                   "('Трансфер из аэропорта', 1500.0), "
                   "('СПА и Сауна', 2000.0), "
                   "('Услуги прачечной', 500.0)");
    }

    query.exec("SELECT COUNT(*) FROM guests");
    if (query.next() && query.value(0).toInt() == 0) {
        query.exec("INSERT INTO guests (full_name, passport, phone) VALUES "
                   "('Иванов Иван Иванович', '4510 123456', '+7 (999) 111-22-33'), "
                   "('Петров Петр Сергеевич', '4512 654321', '+7 (999) 444-55-66')");
    }
}

static Room parseRoomFromQuery(const QSqlQuery &query) {
    Room room;
    room.id = query.value(0).toInt();
    room.roomNumber = query.value(1).toInt();
    room.type = query.value(2).toString();
    room.pricePerNight = query.value(3).toDouble();
    room.status = query.value(4).toString();
    return room;
}

QList<Room> DBManager::getAllRooms() {
    autoCompleteFinishedBookings();
    QList<Room> rooms;
    QSqlQuery query("SELECT id, room_number, type, price_per_night, status FROM rooms ORDER BY id ASC");
    while (query.next()) {
        rooms.append(parseRoomFromQuery(query));
    }
    return rooms;
}

bool DBManager::addRoom(const Room &room) {
    QSqlQuery query;
    query.prepare("INSERT INTO rooms (room_number, type, price_per_night, status) "
                  "VALUES (:number, :type, :price, :status)");
    query.bindValue(":number", room.roomNumber);
    query.bindValue(":type", room.type);
    query.bindValue(":price", room.pricePerNight);
    query.bindValue(":status", room.status);
    return query.exec();
}

bool DBManager::updateRoom(const Room &room) {
    QSqlQuery query;
    query.prepare("UPDATE rooms SET room_number = :number, type = :type, price_per_night = :price "
                  "WHERE id = :id");
    query.bindValue(":number", room.roomNumber);
    query.bindValue(":type", room.type);
    query.bindValue(":price", room.pricePerNight);
    query.bindValue(":id", room.id);
    return query.exec();
}

bool DBManager::deleteRoom(int roomId, QString &errorMessage) {
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM bookings WHERE room_id = :id AND status = 'Активно'");
    checkQuery.bindValue(":id", roomId);

    if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        errorMessage = "Невозможно удалить номер, так как для него есть активные бронирования!";
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM rooms WHERE id = :id");
    query.bindValue(":id", roomId);
    return query.exec();
}

QList<Room> DBManager::getAvailableRoomsForDates(const QDate &checkIn, const QDate &checkOut, int excludeBookingId) {
    autoCompleteFinishedBookings();
    QList<Room> availableRooms;

    QString sql = "SELECT id, room_number, type, price_per_night, status FROM rooms "
                  "WHERE id NOT IN ("
                  "  SELECT room_id FROM bookings "
                  "  WHERE status = 'Активно' AND check_in < :checkOut AND check_out > :checkIn";

    if (excludeBookingId > 0) {
        sql += " AND id != :excludeBookingId";
    }
    sql += ") ORDER BY id ASC";

    QSqlQuery query;
    query.prepare(sql);
    query.bindValue(":checkIn", checkIn.toString(Qt::ISODate));
    query.bindValue(":checkOut", checkOut.toString(Qt::ISODate));
    if (excludeBookingId > 0) {
        query.bindValue(":excludeBookingId", excludeBookingId);
    }

    if (query.exec()) {
        while (query.next()) {
            availableRooms.append(parseRoomFromQuery(query));
        }
    }
    return availableRooms;
}

QList<Service> DBManager::getAllServices() {
    QList<Service> services;
    QSqlQuery query("SELECT id, name, price FROM services ORDER BY id ASC");
    while (query.next()) {
        Service service;
        service.id = query.value(0).toInt();
        service.name = query.value(1).toString();
        service.price = query.value(2).toDouble();
        services.append(service);
    }
    return services;
}

bool DBManager::addService(const Service &service) {
    QSqlQuery query;
    query.prepare("INSERT INTO services (name, price) VALUES (:name, :price)");
    query.bindValue(":name", service.name);
    query.bindValue(":price", service.price);
    return query.exec();
}

bool DBManager::updateService(const Service &service) {
    QSqlQuery query;
    query.prepare("UPDATE services SET name = :name, price = :price WHERE id = :id");
    query.bindValue(":name", service.name);
    query.bindValue(":price", service.price);
    query.bindValue(":id", service.id);
    return query.exec();
}

bool DBManager::deleteService(int serviceId, QString &errorMessage) {
    m_db.transaction();

    QSqlQuery delServicesQuery;
    delServicesQuery.prepare("DELETE FROM booking_services WHERE service_id = :id");
    delServicesQuery.bindValue(":id", serviceId);

    QSqlQuery delQuery;
    delQuery.prepare("DELETE FROM services WHERE id = :id");
    delQuery.bindValue(":id", serviceId);

    if (delServicesQuery.exec() && delQuery.exec()) {
        m_db.commit();
        return true;
    }

    m_db.rollback();
    errorMessage = "Ошибка при удалении услуги из базы данных.";
    return false;
}

QList<Guest> DBManager::getAllGuests() {
    QList<Guest> guests;
    QSqlQuery query("SELECT id, full_name, passport, phone FROM guests ORDER BY id ASC");
    while (query.next()) {
        Guest guest;
        guest.id = query.value(0).toInt();
        guest.fullName = query.value(1).toString();
        guest.passport = query.value(2).toString();
        guest.phone = query.value(3).toString();
        guests.append(guest);
    }
    return guests;
}

int DBManager::addOrGetGuest(const Guest &guest) {
    QSqlQuery query;
    query.prepare("SELECT id FROM guests WHERE passport = :passport");
    query.bindValue(":passport", guest.passport);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    query.prepare("INSERT INTO guests (full_name, passport, phone) VALUES (:name, :passport, :phone)");
    query.bindValue(":name", guest.fullName);
    query.bindValue(":passport", guest.passport);
    query.bindValue(":phone", guest.phone);

    if (query.exec()) {
        return query.lastInsertId().toInt();
    }
    return -1;
}

static Booking parseBookingFromQuery(QSqlQuery &query) {
    Booking booking;
    booking.id = query.value(0).toInt();
    booking.guestId = query.value(1).toInt();
    booking.roomId = query.value(2).toInt();
    booking.checkIn = QDate::fromString(query.value(3).toString(), Qt::ISODate);
    booking.checkOut = QDate::fromString(query.value(4).toString(), Qt::ISODate);
    booking.status = query.value(5).toString();
    booking.totalCost = query.value(6).toDouble();
    booking.guestName = query.value(7).toString();
    booking.roomNumber = query.value(8).toInt();

    QSqlQuery serviceQuery;
    serviceQuery.prepare("SELECT service_id FROM booking_services WHERE booking_id = :id");
    serviceQuery.bindValue(":id", booking.id);

    if (serviceQuery.exec()) {
        while (serviceQuery.next()) {
            booking.serviceIds.append(serviceQuery.value(0).toInt());
        }
    }
    return booking;
}

QList<Booking> DBManager::getAllBookings() {
    autoCompleteFinishedBookings();
    QList<Booking> bookings;
    QSqlQuery query(
        "SELECT b.id, b.guest_id, b.room_id, b.check_in, b.check_out, b.status, b.total_cost, "
        "g.full_name, r.room_number "
        "FROM bookings b "
        "JOIN guests g ON b.guest_id = g.id "
        "JOIN rooms r ON b.room_id = r.id "
        "WHERE b.status = 'Активно' "
        "ORDER BY b.check_in ASC"
    );

    while (query.next()) {
        bookings.append(parseBookingFromQuery(query));
    }
    return bookings;
}

Booking DBManager::getBookingById(int bookingId) {
    QSqlQuery query;
    query.prepare(
        "SELECT b.id, b.guest_id, b.room_id, b.check_in, b.check_out, b.status, b.total_cost, "
        "g.full_name, r.room_number "
        "FROM bookings b "
        "JOIN guests g ON b.guest_id = g.id "
        "JOIN rooms r ON b.room_id = r.id "
        "WHERE b.id = :id"
    );
    query.bindValue(":id", bookingId);

    if (query.exec() && query.next()) {
        return parseBookingFromQuery(query);
    }
    return Booking();
}

bool DBManager::addBooking(Booking &booking) {
    m_db.transaction();

    QSqlQuery query;
    query.prepare("INSERT INTO bookings (guest_id, room_id, check_in, check_out, status, total_cost) "
                  "VALUES (:guest, :room, :checkIn, :checkOut, :status, :cost)");
    query.bindValue(":guest", booking.guestId);
    query.bindValue(":room", booking.roomId);
    query.bindValue(":checkIn", booking.checkIn.toString(Qt::ISODate));
    query.bindValue(":checkOut", booking.checkOut.toString(Qt::ISODate));
    query.bindValue(":status", booking.status);
    query.bindValue(":cost", booking.totalCost);

    if (!query.exec()) {
        m_db.rollback();
        return false;
    }

    booking.id = query.lastInsertId().toInt();

    for (int serviceId : booking.serviceIds) {
        QSqlQuery serviceQuery;
        serviceQuery.prepare("INSERT INTO booking_services (booking_id, service_id) VALUES (:bId, :sId)");
        serviceQuery.bindValue(":bId", booking.id);
        serviceQuery.bindValue(":sId", serviceId);

        if (!serviceQuery.exec()) {
            m_db.rollback();
            return false;
        }
    }

    m_db.commit();
    return true;
}

bool DBManager::updateBooking(const Booking &booking) {
    m_db.transaction();

    QSqlQuery query;
    query.prepare("UPDATE bookings SET room_id = :room, check_in = :checkIn, check_out = :checkOut, total_cost = :cost "
                  "WHERE id = :id");
    query.bindValue(":room", booking.roomId);
    query.bindValue(":checkIn", booking.checkIn.toString(Qt::ISODate));
    query.bindValue(":checkOut", booking.checkOut.toString(Qt::ISODate));
    query.bindValue(":cost", booking.totalCost);
    query.bindValue(":id", booking.id);

    QSqlQuery delQuery;
    delQuery.prepare("DELETE FROM booking_services WHERE booking_id = :id");
    delQuery.bindValue(":id", booking.id);

    if (!query.exec() || !delQuery.exec()) {
        m_db.rollback();
        return false;
    }

    for (int serviceId : booking.serviceIds) {
        QSqlQuery serviceQuery;
        serviceQuery.prepare("INSERT INTO booking_services (booking_id, service_id) VALUES (:bId, :sId)");
        serviceQuery.bindValue(":bId", booking.id);
        serviceQuery.bindValue(":sId", serviceId);

        if (!serviceQuery.exec()) {
            m_db.rollback();
            return false;
        }
    }

    m_db.commit();
    return true;
}

bool DBManager::cancelBooking(int bookingId) {
    QSqlQuery query;
    query.prepare("UPDATE bookings SET status = 'Отменено' WHERE id = :id");
    query.bindValue(":id", bookingId);
    return query.exec();
}

QList<Booking> DBManager::getGuestHistory(int guestId) {
    autoCompleteFinishedBookings();
    QList<Booking> history;
    QSqlQuery query;
    query.prepare(
        "SELECT b.id, b.guest_id, b.room_id, b.check_in, b.check_out, b.status, b.total_cost, "
        "g.full_name, r.room_number "
        "FROM bookings b "
        "JOIN guests g ON b.guest_id = g.id "
        "JOIN rooms r ON b.room_id = r.id "
        "WHERE b.guest_id = :guestId "
        "ORDER BY b.check_in DESC"
    );
    query.bindValue(":guestId", guestId);

    if (query.exec()) {
        while (query.next()) {
            history.append(parseBookingFromQuery(query));
        }
    }
    return history;
}