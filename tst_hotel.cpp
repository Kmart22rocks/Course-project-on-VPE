#include <QtTest>
#include <QSignalSpy>
#include <QFile>
#include "dbmanager.h"
#include "bookingcalculator.h"
#include "currencyapiclient.h"
#include "networkservice.h"
#include "models.h"

class TestHotelLogic : public QObject {
    Q_OBJECT

private:
    const QString testDbPath = "test_hotel.db";

private slots:
    void initTestCase() {
        QFile::remove(testDbPath);
    }

    void cleanupTestCase() {
        QFile::remove(testDbPath);
    }

    void testBookingCalculatorLogic() {
        QDate in(2026, 5, 10);
        QDate out(2026, 5, 14);
        QCOMPARE(BookingCalculator::calculateNights(in, out), 4);
        QCOMPARE(BookingCalculator::calculateNights(out, in), 1);
        QCOMPARE(BookingCalculator::calculateNights(QDate(), out), 1);

        QList<Service> services = {
            {1, "Завтрак", 600.0},
            {2, "Трансфер", 1500.0},
            {3, "Спа", 2000.0}
        };
        QList<int> selected = {1, 3};
        QCOMPARE(BookingCalculator::calculateServicesCost(selected, services), 2600.0);

        double total = BookingCalculator::calculateTotalCost(in, out, 3000.0, selected, services);
        QCOMPARE(total, 4 * 3000.0 + 2600.0);
        QCOMPARE(BookingCalculator::calculateTotalCost(in, out, -100.0, selected, services), 0.0);

        QCOMPARE(BookingCalculator::determineBookingStatus(in, QDate(2026, 5, 10)), QString("Проживает"));
        QCOMPARE(BookingCalculator::determineBookingStatus(in, QDate(2026, 5, 1)), QString("Подтверждено"));

        QCOMPARE(BookingCalculator::convertCurrency(9000.0, 90.0), 100.0);
        QCOMPARE(BookingCalculator::convertCurrency(9000.0, 0.0), 0.0);
    }

    void testGuestStatsAndCategoryRevenue() {
        QList<Room> rooms = {
            {1, 101, "Стандарт", 2500.0, "Свободен"},
            {2, 201, "Люкс", 7500.0, "Свободен"}
        };
        QList<Booking> history = {
            {1, 1, 1, 101, QDate(2026, 5, 1), QDate(2026, 5, 3), "Завершено", "Иванов", 5000.0, {}},
            {2, 1, 2, 201, QDate(2026, 5, 5), QDate(2026, 5, 6), "Отменено", "Иванов", 7500.0, {}},
            {3, 1, 2, 201, QDate(2026, 5, 7), QDate(2026, 5, 9), "Проживает", "Иванов", 15000.0, {}}
        };

        GuestStats stats = BookingCalculator::calculateGuestStats(history);
        QCOMPARE(stats.totalBookings, 3);
        QCOMPARE(stats.activeOrCompletedVisits, 2);
        QCOMPARE(stats.totalSpent, 20000.0);

        QMap<QString, double> rev = BookingCalculator::calculateRevenueByCategory(rooms, history);
        QCOMPARE(rev["Стандарт"], 5000.0);
        QCOMPARE(rev["Люкс"], 15000.0);
    }

    void testCurrencyApiJsonParsing() {
        CurrencyApiClient api;
        QByteArray validJson = R"({
            "Valute": {
                "USD": { "Value": 91.25 },
                "EUR": { "Value": 99.50 }
            }
        })";

        QVERIFY(api.parseRatesJson(validJson));
        QCOMPARE(api.usdRate(), 91.25);
        QCOMPARE(api.eurRate(), 99.50);

        QByteArray invalidJson = R"({"invalid": true})";
        QVERIFY(!api.parseRatesJson(invalidJson));
        QVERIFY(!api.parseRatesJson("not a json"));
    }

    void testDatabaseCrudAndDateOverlap() {
        DBManager db;
        QVERIFY(db.openDatabase(testDbPath));

        QCOMPARE(db.getAllRooms().size(), 6);
        QCOMPARE(db.getAllServices().size(), 5);

        // Тест CRUD номеров
        Room extraRoom{0, 505, "Люкс", 8000.0, "Свободен"};
        QVERIFY(db.addRoom(extraRoom));
        Room addedRoom = db.getAllRooms().last();
        addedRoom.pricePerNight = 8500.0;
        QVERIFY(db.updateRoom(addedRoom));
        QString errMsg;
        QVERIFY(db.deleteRoom(addedRoom.id, errMsg));

        // Тест CRUD услуг
        QVERIFY(db.addService({0, "Поздний выезд", 1000.0}));
        Service addedSrv = db.getAllServices().first();
        addedSrv.price = 1100.0;
        QVERIFY(db.updateService(addedSrv));
        QVERIFY(db.deleteService(addedSrv.id, errMsg));

        // Дедупликация гостей
        int gid1 = db.addOrGetGuest({0, "Иванов И.И.", "4010 111222", "+79001112233"});
        int gid2 = db.addOrGetGuest({0, "Иванов И.И.", "4010 111222", "+79001112233"});
        QCOMPARE(gid1, gid2);

        // Добавление и редактирование бронирования
        int roomId = db.getAllRooms().first().id;
        QDate inDate = QDate::currentDate().addDays(2);
        QDate outDate = QDate::currentDate().addDays(5);

        Booking b{0, gid1, roomId, 101, inDate, outDate, "Подтверждено", "Иванов И.И.", 8100.0, {1}};
        QVERIFY(db.addBooking(b));
        QVERIFY(b.id > 0);

        Booking fetched = db.getBookingById(b.id);
        QCOMPARE(fetched.totalCost, 8100.0);
        fetched.totalCost = 9500.0;
        QVERIFY(db.updateBooking(fetched));

        // Защита от удаления занятого номера
        QVERIFY(!db.deleteRoom(roomId, errMsg));

        // Проверка пересечения дат
        QList<Room> avail = db.getAvailableRoomsForDates(inDate.addDays(1), outDate);
        QCOMPARE(avail.size(), 5);
        QList<Room> availSelf = db.getAvailableRoomsForDates(inDate.addDays(1), outDate, b.id);
        QCOMPARE(availSelf.size(), 6);
    }

    void testTcpServerAndClientWithSignalSpy() {
        DBManager db;
        QVERIFY(db.openDatabase(testDbPath));

        HotelTcpServer server(&db);
        QVERIFY(server.startServer(0));
        QVERIFY(server.isListening());
        quint16 port = server.serverPort();
        QVERIFY(port > 0);

        HotelTcpClient client;
        QSignalSpy serverSpy(&server, &HotelTcpServer::requestProcessed);
        QSignalSpy clientSpy(&client, &HotelTcpClient::summaryReceived);

        client.requestHotelSummary("127.0.0.1", port);

        QVERIFY(clientSpy.wait(2000));
        QCOMPARE(serverSpy.count(), 1);
        QCOMPARE(clientSpy.count(), 1);

        QList<QVariant> args = clientSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 6); // 6 номеров в базе
        QCOMPARE(args.at(1).toInt(), 1); // 1 активная бронь
        QCOMPARE(args.at(2).toInt(), 1); // 1 гость

        // Завершение и отмена бронирования
        int bid = db.getAllBookings().first().id;
        QVERIFY(db.cancelBooking(bid));
        QCOMPARE(db.getGuestHistory(db.getAllGuests().first().id).size(), 1);
    }
};

QTEST_MAIN(TestHotelLogic)
#include "tst_hotel.moc"