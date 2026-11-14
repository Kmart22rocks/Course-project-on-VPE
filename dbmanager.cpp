#include "dbmanager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

DBManager::DBManager(const QString &dbName) : m_dbName(dbName) {
}

DBManager::~DBManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DBManager::openDatabase() {
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection)) {
        m_db = QSqlDatabase::database(QSqlDatabase::defaultConnection);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    }
    m_db.setDatabaseName(m_dbName);

    if (!m_db.open()) {
        return false;
    }

    QSqlQuery pragma;
    pragma.exec("PRAGMA foreign_keys = ON;");
    return createTables();
}

bool DBManager::createTables() {
    QSqlQuery query;

    if (!query.exec("CREATE TABLE IF NOT EXISTS rooms ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "room_number INTEGER UNIQUE NOT NULL, "
                    "type TEXT NOT NULL, "
                    "price_per_night REAL NOT NULL, "
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
    QSqlQuery q("SELECT id, room_number, type, price_per_night, status FROM rooms ORDER BY room_number ASC");
    if (!q.exec()) {
        q.exec("SELECT id, room_number, type, price, status FROM rooms ORDER BY room_number ASC");
    }
    while (q.next()) {
        list.append({q.value(0).toInt(),
                     q.value(1).toInt(),
                     q.value(2).toString(),
                     q.value(3).toDouble(),
                     q.value(4).toString()});
    }
    return list;
}

bool DBManager::addRoom(const Room &room) {
    QSqlQuery q;
    q.prepare("INSERT INTO rooms (room_number, type, price_per_night, status) VALUES (:num, :type, :price, :status)");
    q.bindValue(":num", room.roomNumber);
    q.bindValue(":type", room.type);
    q.bindValue(":price", room.pricePerNight);
    q.bindValue(":status", room.status);
    if (q.exec()) return true;

    q.prepare("INSERT INTO rooms (room_number, type, price, status) VALUES (:num, :type, :price, :status)");
    q.bindValue(":num", room.roomNumber);
    q.bindValue(":type", room.type);
    q.bindValue(":price", room.pricePerNight);
    q.bindValue(":status", room.status);
    return q.exec();
}

bool DBManager::updateRoom(const Room &room) {
    QSqlQuery q;
    q.prepare("UPDATE rooms SET room_number = :num, type = :type, price_per_night = :price, status = :status WHERE id = :id");
    q.bindValue(":num", room.roomNumber);
    q.bindValue(":type", room.type);
    q.bindValue(":price", room.pricePerNight);
    q.bindValue(":status", room.status);
    q.bindValue(":id", room.id);
    if (q.exec()) return true;

    q.prepare("UPDATE rooms SET room_number = :num, type = :type, price = :price, status = :status WHERE id = :id");
    q.bindValue(":num", room.roomNumber);
    q.bindValue(":type", room.type);
    q.bindValue(":price", room.pricePerNight);
    q.bindValue(":status", room.status);
    q.bindValue(":id", room.id);
    return q.exec();
}

bool DBManager::deleteRoom(int roomId, QString &errorMsg) {
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM bookings WHERE room_id = :rid AND status IN ('Активно', 'Забронировано')");
    check.bindValue(":rid", roomId);
    if (check.exec() && check.next() && check.value(0).toInt() > 0) {
        errorMsg = "Невозможно удалить номер, к которому привязаны активные или будущие бронирования.";
        return false;
    }

    QSqlQuery q;
    q.prepare("DELETE FROM rooms WHERE id = :id");
    q.bindValue(":id", roomId);
    if (!q.exec()) {
        errorMsg = q.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::deleteRoom(int roomId) {
    QString err;
    return deleteRoom(roomId, err);
}

bool DBManager::isRoomAvailable(int roomId, const QDate &checkIn, const QDate &checkOut, int excludeBookingId) {
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM bookings "
              "WHERE room_id = :rid AND id != :exId "
              "AND status NOT IN ('Отменено', 'Завершено') "
              "AND check_in < :out AND check_out > :in");
    q.bindValue(":rid", roomId);
    q.bindValue(":exId", excludeBookingId);
    q.bindValue(":in", checkIn.toString(Qt::ISODate));
    q.bindValue(":out", checkOut.toString(Qt::ISODate));
    if (q.exec() && q.next()) {
        return q.value(0).toInt() == 0;
    }
    return false;
}

QList<Service> DBManager::getAllServices() {
    QList<Service> list;
    QSqlQuery q("SELECT id, name, price FROM services ORDER BY name ASC");
    while (q.next()) {
        list.append({q.value(0).toInt(), q.value(1).toString(), q.value(2).toDouble()});
    }
    return list;
}

bool DBManager::addService(const Service &service) {
    QSqlQuery q;
    q.prepare("INSERT INTO services (name, price) VALUES (:name, :price)");
    q.bindValue(":name", service.name);
    q.bindValue(":price", service.price);
    return q.exec();
}

bool DBManager::updateService(const Service &service) {
    QSqlQuery q;
    q.prepare("UPDATE services SET name = :name, price = :price WHERE id = :id");
    q.bindValue(":name", service.name);
    q.bindValue(":price", service.price);
    q.bindValue(":id", service.id);
    return q.exec();
}

bool DBManager::deleteService(int serviceId, QString &errorMsg) {
    QSqlQuery q;
    q.prepare("DELETE FROM services WHERE id = :id");
    q.bindValue(":id", serviceId);
    if (!q.exec()) {
        errorMsg = q.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::deleteService(int serviceId) {
    QString err;
    return deleteService(serviceId, err);
}

QList<Guest> DBManager::getAllGuests() {
    QList<Guest> list;
    QSqlQuery q("SELECT id, full_name, passport, phone FROM guests ORDER BY full_name ASC");
    while (q.next()) {
        list.append({q.value(0).toInt(),
                     q.value(1).toString(),
                     q.value(2).toString(),
                     q.value(3).toString()});
    }
    return list;
}

int DBManager::addOrGetGuest(const Guest &guest) {
    QSqlQuery check;
    check.prepare("SELECT id FROM guests WHERE passport = :pass");
    check.bindValue(":pass", guest.passport.trimmed());
    if (check.exec() && check.next()) {
        int existingId = check.value(0).toInt();
        QSqlQuery upd;
        upd.prepare("UPDATE guests SET full_name = :name, phone = :phone WHERE id = :id");
        upd.bindValue(":name", guest.fullName.trimmed());
        upd.bindValue(":phone", guest.phone.trimmed());
        upd.bindValue(":id", existingId);
        upd.exec();
        return existingId;
    }

    QSqlQuery ins;
    ins.prepare("INSERT INTO guests (full_name, passport, phone) VALUES (:name, :pass, :phone)");
    ins.bindValue(":name", guest.fullName.trimmed());
    ins.bindValue(":pass", guest.passport.trimmed());
    ins.bindValue(":phone", guest.phone.trimmed());
    if (ins.exec()) {
        return ins.lastInsertId().toInt();
    }
    return -1;
}

QList<Booking> DBManager::getGuestHistory(int guestId) {
    QList<Booking> list;
    QSqlQuery q;
    q.prepare("SELECT b.id, b.guest_id, b.room_id, r.room_number, g.full_name, "
              "b.check_in, b.check_out, b.status, b.total_cost "
              "FROM bookings b "
              "JOIN rooms r ON b.room_id = r.id "
              "JOIN guests g ON b.guest_id = g.id "
              "WHERE b.guest_id = :gid "
              "ORDER BY b.check_in DESC");
    q.bindValue(":gid", guestId);
    if (q.exec()) {
        while (q.next()) {
            Booking b;
            b.id = q.value(0).toInt();
            b.guestId = q.value(1).toInt();
            b.roomId = q.value(2).toInt();
            b.roomNumber = q.value(3).toInt();
            b.guestName = q.value(4).toString();
            b.checkIn = QDate::fromString(q.value(5).toString(), Qt::ISODate);
            b.checkOut = QDate::fromString(q.value(6).toString(), Qt::ISODate);
            b.status = q.value(7).toString();
            b.totalCost = q.value(8).toDouble();
            b.serviceIds = getBookingServices(b.id);
            list.append(b);
        }
    }
    return list;
}

QList<int> DBManager::getBookingServices(int bookingId) {
    QList<int> ids;
    QSqlQuery q;
    q.prepare("SELECT service_id FROM booking_services WHERE booking_id = :bid");
    q.bindValue(":bid", bookingId);
    if (q.exec()) {
        while (q.next()) {
            ids.append(q.value(0).toInt());
        }
    }
    return ids;
}

QList<Booking> DBManager::getAllBookings() {
    QList<Booking> list;
    QSqlQuery q("SELECT b.id, b.guest_id, b.room_id, r.room_number, g.full_name, "
                "b.check_in, b.check_out, b.status, b.total_cost "
                "FROM bookings b "
                "JOIN rooms r ON b.room_id = r.id "
                "JOIN guests g ON b.guest_id = g.id "
                "WHERE b.status IN ('Активно', 'Забронировано') "
                "ORDER BY b.check_in ASC");
    while (q.next()) {
        Booking b;
        b.id = q.value(0).toInt();
        b.guestId = q.value(1).toInt();
        b.roomId = q.value(2).toInt();
        b.roomNumber = q.value(3).toInt();
        b.guestName = q.value(4).toString();
        b.checkIn = QDate::fromString(q.value(5).toString(), Qt::ISODate);
        b.checkOut = QDate::fromString(q.value(6).toString(), Qt::ISODate);
        b.status = q.value(7).toString();
        b.totalCost = q.value(8).toDouble();
        b.serviceIds = getBookingServices(b.id);
        list.append(b);
    }
    return list;
}

bool DBManager::addBooking(const Booking &booking) {
    m_db.transaction();

    QSqlQuery q;
    q.prepare("INSERT INTO bookings (guest_id, room_id, check_in, check_out, status, total_cost) "
              "VALUES (:gid, :rid, :in, :out, :st, :cost)");
    q.bindValue(":gid", booking.guestId);
    q.bindValue(":rid", booking.roomId);
    q.bindValue(":in", booking.checkIn.toString(Qt::ISODate));
    q.bindValue(":out", booking.checkOut.toString(Qt::ISODate));
    q.bindValue(":st", booking.status);
    q.bindValue(":cost", booking.totalCost);

    if (!q.exec()) {
        m_db.rollback();
        return false;
    }

    int bookingId = q.lastInsertId().toInt();
    for (int sid : booking.serviceIds) {
        QSqlQuery sq;
        sq.prepare("INSERT INTO booking_services (booking_id, service_id) VALUES (:bid, :sid)");
        sq.bindValue(":bid", bookingId);
        sq.bindValue(":sid", sid);
        if (!sq.exec()) {
            m_db.rollback();
            return false;
        }
    }

    return m_db.commit();
}

bool DBManager::updateBooking(const Booking &booking) {
    m_db.transaction();

    QSqlQuery q;
    q.prepare("UPDATE bookings SET guest_id = :gid, room_id = :rid, check_in = :in, "
              "check_out = :out, status = :st, total_cost = :cost WHERE id = :id");
    q.bindValue(":gid", booking.guestId);
    q.bindValue(":rid", booking.roomId);
    q.bindValue(":in", booking.checkIn.toString(Qt::ISODate));
    q.bindValue(":out", booking.checkOut.toString(Qt::ISODate));
    q.bindValue(":st", booking.status);
    q.bindValue(":cost", booking.totalCost);
    q.bindValue(":id", booking.id);

    if (!q.exec()) {
        m_db.rollback();
        return false;
    }

    QSqlQuery del;
    del.prepare("DELETE FROM booking_services WHERE booking_id = :bid");
    del.bindValue(":bid", booking.id);
    if (!del.exec()) {
        m_db.rollback();
        return false;
    }

    for (int sid : booking.serviceIds) {
        QSqlQuery sq;
        sq.prepare("INSERT INTO booking_services (booking_id, service_id) VALUES (:bid, :sid)");
        sq.bindValue(":bid", booking.id);
        sq.bindValue(":sid", sid);
        if (!sq.exec()) {
            m_db.rollback();
            return false;
        }
    }

    return m_db.commit();
}

bool DBManager::updateBookingStatus(int bookingId, const QString &newStatus) {
    QSqlQuery q;
    q.prepare("UPDATE bookings SET status = :st WHERE id = :id");
    q.bindValue(":st", newStatus);
    q.bindValue(":id", bookingId);
    return q.exec();
}

bool DBManager::cancelBooking(int bookingId) {
    return updateBookingStatus(bookingId, "Отменено");
}

bool DBManager::deleteBooking(int bookingId) {
    QSqlQuery q;
    q.prepare("DELETE FROM bookings WHERE id = :id");
    q.bindValue(":id", bookingId);
    return q.exec();
}

void DBManager::autoCompleteFinishedBookings() {
    QSqlQuery q;
    q.prepare("UPDATE bookings SET status = 'Завершено' "
              "WHERE status IN ('Активно', 'Забронировано') AND check_out < :today");
    q.bindValue(":today", QDate::currentDate().toString(Qt::ISODate));
    q.exec();
}