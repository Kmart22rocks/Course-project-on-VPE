#ifndef BOOKINGCALCULATOR_H
#define BOOKINGCALCULATOR_H

#include "models.h"
#include <QDate>
#include <QList>
#include <QMap>
#include <QString>

class BookingCalculator {
public:
    static int calculateNights(const QDate &checkIn, const QDate &checkOut);
    static double calculateServicesCost(const QList<int> &selectedServiceIds,
                                        const QList<Service> &allServices);
    static double calculateTotalCost(const QDate &checkIn,
                                     const QDate &checkOut,
                                     double roomPricePerNight,
                                     const QList<int> &selectedServiceIds,
                                     const QList<Service> &allServices);
    static QString determineBookingStatus(const QDate &checkIn,
                                          const QDate &today = QDate::currentDate());
    static bool isRoomAvailableForDates(int roomId,
                                        const QString &roomStatus,
                                        const QDate &checkIn,
                                        const QDate &checkOut,
                                        const QList<Booking> &bookings,
                                        int ignoreBookingId = -1);
    static GuestStats calculateGuestStats(const QList<Booking> &history);
    static QMap<QString, double> calculateRevenueByCategory(const QList<Room> &rooms,
                                                            const QList<Booking> &bookings);
    static double convertCurrency(double amountRub, double rate);
};

#endif