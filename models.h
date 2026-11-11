#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QDate>
#include <QList>

// Модель гостя
struct Guest {
    int id = 0;
    QString fullName;
    QString passport;
    QString phone;
};

// Модель гостиничного номера
struct Room {
    int id = 0;
    int roomNumber = 0;
    QString type;
    QString status;
    double pricePerNight = 0.0;
};

// Модель дополнительной услуги
struct Service {
    int id = 0;
    QString name;
    double price = 0.0;
};

// Модель бронирования
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

#endif // MODELS_H