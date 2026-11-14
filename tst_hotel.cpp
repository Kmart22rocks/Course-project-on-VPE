#include <QtTest>
#include <QSignalSpy>
#include <QFile>

#include "models.h"
#include "bookingcalculator.h"
#include "currencyapiclient.h"
#include "dbmanager.h"
#include "networkservice.h"

class TestHotelLogic : public QObject {
    Q_OBJECT

private slots:
    void testBookingCalculatorLogic();
    void testRoomAvailabilityLogic();
    void testGuestStatsAndCategoryRevenue();
    void testCurrencyApiJsonParsing();
    void testDatabaseCrudAndTransactions();
    void testTcpServerAndClientWithSignalSpy();
};

void TestHotelLogic::testBookingCalculatorLogic() {
    QDate in(2026, 6, 1);
    QDate out(2026, 6, 5);
    QCOMPARE(BookingCalculator::calculateNights(in, out), 4);
    QCOMPARE(BookingCalculator::calculateNights(out, in), 0);

    QList<Service> allServices = {
        {1, "Завтрак", 600.0},
        {2, "Парковка", 400.0},
        {3, "СПА", 2000.0}
    };

    QList<int> chosen = {1, 3};
    QCOMPARE(BookingCalculator::calculateServicesCost(chosen, allServices), 2600.0);

    double total = BookingCalculator::calculateTotalCost(in, out, 3000.0, chosen, allServices);
    QCOMPARE(total, 14600.0);

    QCOMPARE(BookingCalculator::determineBookingStatus(in, QDate(2026, 6, 1)), QString("Активно"));
    QCOMPARE(BookingCalculator::determineBookingStatus(in, QDate(2026, 5, 20)), QString("Забронировано"));
    QCOMPARE(BookingCalculator::convertCurrency(9000.0, 90.0), 100.0);
}

void TestHotelLogic::testRoomAvailabilityLogic() {
    Booking existing;
    existing.id = 10;
    existing.roomId = 1;
    existing.checkIn = QDate(2026, 7, 10);
    existing.checkOut = QDate(2026, 7, 15);
    existing.status = "Забронировано";

    QList<Booking> bookings = {existing};

    QVERIFY(!BookingCalculator::isRoomAvailableForDates(
        1, "Свободен", QDate(2026, 7, 12), QDate(2026, 7, 18), bookings));
    QVERIFY(BookingCalculator::isRoomAvailableForDates(
        1, "Свободен", QDate(2026, 7, 15), QDate(2026, 7, 20), bookings));
    QVERIFY(BookingCalculator::isRoomAvailableForDates(
        1, "Свободен", QDate(2026, 7, 12), QDate(2026, 7, 18), bookings, 10));
    QVERIFY(!BookingCalculator::isRoomAvailableForDates(
        1, "На ремонте", QDate(2026, 8, 1), QDate(2026, 8, 5), bookings));
}

void TestHotelLogic::testGuestStatsAndCategoryRevenue() {
    QList<Room> rooms = {
        {1, 101, "Стандарт", 2500.0, "Свободен"},
        {2, 201, "Люкс", 7000.0, "Свободен"}
    };

    Booking b1{1, 1, 1, 101, QDate(2026, 5, 1), QDate(2026, 5, 3), "Завершено", "Иванов И.И.", 5000.0, {}};
    Booking b2{2, 1, 2, 201, QDate(2026, 5, 10), QDate(2026, 5, 12), "Отменено", "Иванов И.И.", 14000.0, {}};
    Booking b3{3, 1, 2, 201, QDate(2026, 5, 20), QDate(2026, 5, 22), "Активно", "Иванов И.И.", 14000.0, {}};

    QList<Booking> list = {b1, b2, b3};
    GuestStats stats = BookingCalculator::calculateGuestStats(list);
    QCOMPARE(stats.totalBookings, 3);
    QCOMPARE(stats.activeOrCompletedVisits, 2);
    QCOMPARE(stats.totalSpent, 19000.0);

    QMap<QString, double> rev = BookingCalculator::calculateRevenueByCategory(rooms, list);
    QCOMPARE(rev.value("Стандарт"), 5000.0);
    QCOMPARE(rev.value("Люкс"), 14000.0);
}

void TestHotelLogic::testCurrencyApiJsonParsing() {
    CurrencyApiClient api;
    QSignalSpy spyUpdated(&api, &CurrencyApiClient::ratesUpdated);

    QByteArray sampleJson = R"({
        "Valute": {
            "USD": { "Value": 88.50 },
            "EUR": { "Value": 96.25 }
        }
    })";

    QVERIFY(api.parseRatesJson(sampleJson));
    QCOMPARE(spyUpdated.count(), 1);
    QCOMPARE(api.usdRate(), 88.50);
    QCOMPARE(api.eurRate(), 96.25);
}

void TestHotelLogic::testDatabaseCrudAndTransactions() {
    const QString testDbFile = "test_hotel_temp.db";
    QFile::remove(testDbFile);

    {
        DBManager db(testDbFile);
        QVERIFY(db.openDatabase());

        Room r{0, 101, "Люкс", 5500.0, "Свободен"};
        QVERIFY(db.addRoom(r));
        QList<Room> rooms = db.getAllRooms();
        QCOMPARE(rooms.size(), 1);
        r = rooms.first();
        r.pricePerNight = 6000.0;
        QVERIFY(db.updateRoom(r));

        Service s{0, "Завтрак", 800.0};
        QVERIFY(db.addService(s));
        QList<Service> services = db.getAllServices();
        QCOMPARE(services.size(), 1);
        s = services.first();
        s.price = 900.0;
        QVERIFY(db.updateService(s));

        Guest g{0, "Тестов Тест Тестович", "4010 112233", "+79990001122"};
        int guestId1 = db.addOrGetGuest(g);
        int guestId2 = db.addOrGetGuest(g);
        QVERIFY(guestId1 > 0);
        QCOMPARE(guestId1, guestId2);
        QCOMPARE(db.getAllGuests().size(), 1);

        QDate in1(2026, 7, 10);
        QDate out1(2026, 7, 15);
        QVERIFY(db.isRoomAvailable(r.id, in1, out1));

        Booking b;
        b.guestId = guestId1;
        b.roomId = r.id;
        b.checkIn = in1;
        b.checkOut = out1;
        b.status = "Забронировано";
        b.totalCost = 30900.0;
        b.serviceIds = {s.id};

        QVERIFY(db.addBooking(b));
        QVERIFY(!db.isRoomAvailable(r.id, QDate(2026, 7, 12), QDate(2026, 7, 18)));
        QVERIFY(!db.deleteRoom(r.id));

        QList<Booking> bookings = db.getAllBookings();
        QCOMPARE(bookings.size(), 1);
        b = bookings.first();
        QCOMPARE(b.serviceIds.size(), 1);

        b.totalCost = 31500.0;
        QVERIFY(db.updateBooking(b));
        QVERIFY(db.updateBookingStatus(b.id, "Отменено"));
        QCOMPARE(db.getGuestHistory(guestId1).size(), 1);

        db.autoCompleteFinishedBookings();
        QVERIFY(db.deleteBooking(b.id));
        QVERIFY(db.deleteService(s.id));
        QVERIFY(db.deleteRoom(r.id));
    }

    QFile::remove(testDbFile);
}

void TestHotelLogic::testTcpServerAndClientWithSignalSpy() {
    const QString testDbFile = "test_tcp_temp.db";
    QFile::remove(testDbFile);

    {
        DBManager db(testDbFile);
        QVERIFY(db.openDatabase());
        db.addRoom({0, 101, "Стандарт", 3000.0, "Свободен"});

        HotelTcpServer server(&db);
        QVERIFY(server.startServer(0));
        quint16 port = server.serverPort();
        QVERIFY(port > 0);

        HotelTcpClient client;
        QSignalSpy spyServer(&server, &HotelTcpServer::requestProcessed);
        QSignalSpy spyClient(&client, &HotelTcpClient::summaryReceived);

        client.requestHotelSummary("127.0.0.1", port);

        QVERIFY(spyClient.wait(3000));
        QCOMPARE(spyServer.count(), 1);
        QCOMPARE(spyClient.count(), 1);
    }

    QFile::remove(testDbFile);
}

QTEST_MAIN(TestHotelLogic)
#include "tst_hotel.moc"