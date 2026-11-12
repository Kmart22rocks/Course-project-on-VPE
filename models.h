#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QDate>
#include <QList>
#include <QMap>

struct Guest {
    int id = 0;
    QString fullName;
    QString passport;
    QString phone;
};

struct Room {
    int id = 0;
    int roomNumber = 0;
    QString type;
    double pricePerNight = 0.0;
    QString status;
};

struct Service {
    int id = 0;
    QString name;
    double price = 0.0;
};

struct Booking {
    int id = 0;
    int guestId = 0;
    int roomId = 0;
    int roomNumber = 0;
    QDate checkIn;
    QDate checkOut;
    QString status;
    QString guestName;
    double totalCost = 0.0;
    QList<int> serviceIds;
};

struct GuestStats {
    int totalBookings = 0;
    int activeOrCompletedVisits = 0;
    double totalSpent = 0.0;
};

#endif // MODELS_H