#include "dbmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

DBManager::DBManager(QObject *parent) : QObject(parent) {}

DBManager::~DBManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DBManager::openDatabase(const QString &path) {
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        m_db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    }
    m_db.setDatabaseName(path);

    if (!m_db.open()) return false;

    QSqlQuery pragmaQuery(m_db);
    pragmaQuery.exec("PRAGMA foreign_keys = ON;");

    if (!createTables()) return false;

    autoCompleteFinishedBookings();
    return true;
}

bool DBManager::createTables() {
    QSqlQuery query;

    if (!query.exec("CREATE TABLE IF NOT EXISTS rooms ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "room_number INTEGER UNIQUE NOT NULL, "
                    "type TEXT NOT NULL, "
                    "price REAL NOT NULL, "
                    "status TEXT NOT NULL)")) {
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS services ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "name TEXT UNIQUE NOT NULL, "
                    "price REAL NOT NULL)")) {
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS guests ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "full_name TEXT NOT NULL, "
                    "passport TEXT UNIQUE NOT NULL, "
                    "phone TEXT NOT NULL)")) {
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS bookings ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "guest_id INTEGER NOT NULL, "
                    "room_id INTEGER NOT NULL, "
                    "check_in TEXT NOT NULL, "
                    "check_out TEXT NOT NULL, "
                    "status TEXT NOT NULL, "
                    "total_cost REAL NOT NULL, "
                    "FOREIGN KEY(guest_id) REFERENCES guests(id) ON DELETE CASCADE, "
                    "FOREIGN KEY(room_id) REFERENCES rooms(id) ON DELETE RESTRICT)")) {
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS booking_services ("
                    "booking_id INTEGER NOT NULL, "
                    "service_id INTEGER NOT NULL, "
                    "PRIMARY KEY(booking_id, service_id), "
                    "FOREIGN KEY(booking_id) REFERENCES bookings(id) ON DELETE CASCADE, "
                    "FOREIGN KEY(service_id) REFERENCES services(id) ON DELETE CASCADE)")) {
        return false;
    }

    return true;
}

QList<Room> DBManager::getAllRooms() {
    QList<Room> list;
    QSqlQuery q("SELECT id, room_number, type, price_per_night, status FROM rooms ORDER BY room_number ASC", m_db);
    while (q.next()) {
        list.append({
            q.value(0).toInt(),
            q.value(1).toInt(),
            q.value(2).toString(),
            q.value(3).toDouble(),
            q.value(4).toString()
        });
    }
    return list;
}

bool DBManager::addRoom(const Room &room) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO rooms (room_number, type, price_per_night, status) VALUES (:num, :type, :price, :status);");
    q.bindValue(":num", room.roomNumber);
    q.bindValue(":type", room.type);
    q.bindValue(":price", room.pricePerNight);
    q.bindValue(":status", room.status);
    return q.exec();
}

bool DBManager::updateRoom(const Room &room) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE rooms SET room_number = :num, type = :type, price_per_night = :price, status = :status WHERE id = :id;");
    q.bindValue(":num", room.roomNumber);
    q.bindValue(":type", room.type);
    q.bindValue(":price", room.pricePerNight);
    q.bindValue(":status", room.status);
    q.bindValue(":id", room.id);
    return q.exec();
}

bool DBManager::deleteRoom(int roomId, QString &errorMsg) {
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT COUNT(*) FROM bookings WHERE room_id = :room_id AND status != 'Отменено';");
    checkQuery.bindValue(":room_id", roomId);
    if (checkQuery.exec() && checkQuery.next()) {
        if (checkQuery.value(0).toInt() > 0) {
            errorMsg = "Невозможно удалить номер, так как к нему привязаны активные или будущие бронирования!";
            return false;
        }
    }
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM rooms WHERE id = :id;");
    q.bindValue(":id", roomId);
    return q.exec();
}

QList<Room> DBManager::getAvailableRoomsForDates(const QDate &checkIn, const QDate &checkOut, int excludeBookingId) {
    QList<Room> rooms;
    QSqlQuery q(m_db);
    QString sql =
        "SELECT id, room_number, type, price_per_night, status FROM rooms "
        "WHERE status != 'Ремонт' AND id NOT IN ("
        "    SELECT room_id FROM bookings "
        "    WHERE status IN ('Подтверждено', 'Проживает') "
        "      AND check_in < :check_out "
        "      AND check_out > :check_in ";
    if (excludeBookingId > 0) {
        sql += " AND id != :exclude_id";
    }
    sql += ") ORDER BY room_number ASC";

    q.prepare(sql);
    q.bindValue(":check_in", checkIn.toString(Qt::ISODate));
    q.bindValue(":check_out", checkOut.toString(Qt::ISODate));
    if (excludeBookingId > 0) {
        q.bindValue(":exclude_id", excludeBookingId);
    }

    if (q.exec()) {
        while (q.next()) {
            rooms.append({
                q.value(0).toInt(),
                q.value(1).toInt(),
                q.value(2).toString(),
                q.value(3).toDouble(),
                q.value(4).toString()
            });
        }
    }
    return rooms;
}

QList<Service> DBManager::getAllServices() {
    QList<Service> list;
    QSqlQuery q("SELECT id, name, price FROM services ORDER BY name ASC", m_db);
    while (q.next()) {
        list.append({q.value(0).toInt(), q.value(1).toString(), q.value(2).toDouble()});
    }
    return list;
}

bool DBManager::addService(const Service &service) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO services (name, price) VALUES (:name, :price);");
    q.bindValue(":name", service.name);
    q.bindValue(":price", service.price);
    return q.exec();
}

bool DBManager::updateService(const Service &service) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE services SET name = :name, price = :price WHERE id = :id;");
    q.bindValue(":name", service.name);
    q.bindValue(":price", service.price);
    q.bindValue(":id", service.id);
    return q.exec();
}

bool DBManager::deleteService(int serviceId, QString &errorMsg) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM services WHERE id = :id;");
    q.bindValue(":id", serviceId);
    if (!q.exec()) {
        errorMsg = q.lastError().text();
        return false;
    }
    return true;
}

QList<Guest> DBManager::getAllGuests() {
    QList<Guest> list;
    QSqlQuery q("SELECT id, full_name, passport, phone FROM guests ORDER BY full_name ASC", m_db);
    while (q.next()) {
        list.append({q.value(0).toInt(), q.value(1).toString(), q.value(2).toString(), q.value(3).toString()});
    }
    return list;
}

int DBManager::addOrGetGuest(const Guest &guest) {
    QSqlQuery check(m_db);
    check.prepare("SELECT id FROM guests WHERE passport = :pass;");
    check.bindValue(":pass", guest.passport);
    if (check.exec() && check.next()) {
        return check.value(0).toInt();
    }
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO guests (full_name, passport, phone) VALUES (:name, :pass, :phone);");
    q.bindValue(":name", guest.fullName);
    q.bindValue(":pass", guest.passport);
    q.bindValue(":phone", guest.phone);
    if (q.exec()) {
        return q.lastInsertId().toInt();
    }
    return -1;
}

QList<Booking> DBManager::getAllBookings() {
    autoCompleteFinishedBookings();
    QList<Booking> list;
    QSqlQuery q(
        "SELECT b.id, b.guest_id, b.room_id, r.room_number, b.check_in, b.check_out, "
        "b.total_cost, b.status, g.full_name "
        "FROM bookings b "
        "JOIN guests g ON b.guest_id = g.id "
        "JOIN rooms r ON b.room_id = r.id "
        "WHERE b.status IN ('Подтверждено', 'Проживает') "
        "ORDER BY b.check_in ASC", m_db
    );
    while (q.next()) {
        Booking b;
        b.id = q.value(0).toInt();
        b.guestId = q.value(1).toInt();
        b.roomId = q.value(2).toInt();
        b.roomNumber = q.value(3).toInt();
        b.checkIn = QDate::fromString(q.value(4).toString(), Qt::ISODate);
        b.checkOut = QDate::fromString(q.value(5).toString(), Qt::ISODate);
        b.totalCost = q.value(6).toDouble();
        b.status = q.value(7).toString();
        b.guestName = q.value(8).toString();
        list.append(b);
    }
    return list;
}

Booking DBManager::getBookingById(int bookingId) {
    Booking b;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT b.id, b.guest_id, b.room_id, r.room_number, b.check_in, b.check_out, "
        "b.total_cost, b.status, g.full_name "
        "FROM bookings b "
        "JOIN guests g ON b.guest_id = g.id "
        "JOIN rooms r ON b.room_id = r.id "
        "WHERE b.id = :id"
    );
    q.bindValue(":id", bookingId);
    if (q.exec() && q.next()) {
        b.id = q.value(0).toInt();
        b.guestId = q.value(1).toInt();
        b.roomId = q.value(2).toInt();
        b.roomNumber = q.value(3).toInt();
        b.checkIn = QDate::fromString(q.value(4).toString(), Qt::ISODate);
        b.checkOut = QDate::fromString(q.value(5).toString(), Qt::ISODate);
        b.totalCost = q.value(6).toDouble();
        b.status = q.value(7).toString();
        b.guestName = q.value(8).toString();

        QSqlQuery sq(m_db);
        sq.prepare("SELECT service_id FROM booking_services WHERE booking_id = :bid;");
        sq.bindValue(":bid", b.id);
        if (sq.exec()) {
            while (sq.next()) {
                b.serviceIds.append(sq.value(0).toInt());
            }
        }
    }
    return b;
}

bool DBManager::addBooking(Booking &booking) {
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO bookings (guest_id, room_id, check_in, check_out, total_cost, status) "
        "VALUES (:gid, :rid, :in, :out, :cost, :status);"
    );
    q.bindValue(":gid", booking.guestId);
    q.bindValue(":rid", booking.roomId);
    q.bindValue(":in", booking.checkIn.toString(Qt::ISODate));
    q.bindValue(":out", booking.checkOut.toString(Qt::ISODate));
    q.bindValue(":cost", booking.totalCost);
    q.bindValue(":status", booking.status);

    if (!q.exec()) return false;

    booking.id = q.lastInsertId().toInt();

    for (int sid : booking.serviceIds) {
        QSqlQuery sq(m_db);
        sq.prepare("INSERT INTO booking_services (booking_id, service_id) VALUES (:bid, :sid);");
        sq.bindValue(":bid", booking.id);
        sq.bindValue(":sid", sid);
        sq.exec();
    }
    return true;
}

bool DBManager::updateBooking(const Booking &booking) {
    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE bookings SET room_id = :rid, check_in = :in, check_out = :out, "
        "total_cost = :cost, status = :status WHERE id = :id;"
    );
    q.bindValue(":rid", booking.roomId);
    q.bindValue(":in", booking.checkIn.toString(Qt::ISODate));
    q.bindValue(":out", booking.checkOut.toString(Qt::ISODate));
    q.bindValue(":cost", booking.totalCost);
    q.bindValue(":status", booking.status);
    q.bindValue(":id", booking.id);

    if (!q.exec()) return false;

    QSqlQuery delServices(m_db);
    delServices.prepare("DELETE FROM booking_services WHERE booking_id = :bid;");
    delServices.bindValue(":bid", booking.id);
    delServices.exec();

    for (int sid : booking.serviceIds) {
        QSqlQuery sq(m_db);
        sq.prepare("INSERT INTO booking_services (booking_id, service_id) VALUES (:bid, :sid);");
        sq.bindValue(":bid", booking.id);
        sq.bindValue(":sid", sid);
        sq.exec();
    }
    return true;
}

bool DBManager::cancelBooking(int bookingId) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE bookings SET status = 'Отменено' WHERE id = :id;");
    q.bindValue(":id", bookingId);
    return q.exec();
}

QList<Booking> DBManager::getGuestHistory(int guestId) {
    QList<Booking> list;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT b.id, b.guest_id, b.room_id, r.room_number, b.check_in, b.check_out, "
        "b.total_cost, b.status, g.full_name "
        "FROM bookings b "
        "JOIN guests g ON b.guest_id = g.id "
        "JOIN rooms r ON b.room_id = r.id "
        "WHERE b.guest_id = :gid ORDER BY b.check_in DESC;"
    );
    q.bindValue(":gid", guestId);
    if (q.exec()) {
        while (q.next()) {
            Booking b;
            b.id = q.value(0).toInt();
            b.guestId = q.value(1).toInt();
            b.roomId = q.value(2).toInt();
            b.roomNumber = q.value(3).toInt();
            b.checkIn = QDate::fromString(q.value(4).toString(), Qt::ISODate);
            b.checkOut = QDate::fromString(q.value(5).toString(), Qt::ISODate);
            b.totalCost = q.value(6).toDouble();
            b.status = q.value(7).toString();
            b.guestName = q.value(8).toString();
            list.append(b);
        }
    }
    return list;
}

void DBManager::autoCompleteFinishedBookings() {
    QSqlQuery q(m_db);
    q.prepare("UPDATE bookings SET status = 'Завершено' WHERE check_out <= :today AND status IN ('Подтверждено', 'Проживает');");
    q.bindValue(":today", QDate::currentDate().toString(Qt::ISODate));
    q.exec();
}