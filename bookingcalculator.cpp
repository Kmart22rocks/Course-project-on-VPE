#include "bookingcalculator.h"

int BookingCalculator::calculateNights(const QDate &checkIn, const QDate &checkOut) {
    if (!checkIn.isValid() || !checkOut.isValid()) {
        return 1;
    }
    int nights = checkIn.daysTo(checkOut);
    return (nights > 0) ? nights : 1;
}

double BookingCalculator::calculateServicesCost(const QList<int> &selectedServiceIds,
                                                const QList<Service> &allServices) {
    double servicesCost = 0.0;
    for (int sid : selectedServiceIds) {
        for (const auto &s : allServices) {
            if (s.id == sid) {
                servicesCost += s.price;
                break;
            }
        }
    }
    return servicesCost;
}

double BookingCalculator::calculateTotalCost(const QDate &checkIn,
                                             const QDate &checkOut,
                                             double roomPricePerNight,
                                             const QList<int> &selectedServiceIds,
                                             const QList<Service> &allServices) {
    if (roomPricePerNight < 0.0) {
        return 0.0;
    }
    int nights = calculateNights(checkIn, checkOut);
    double servicesCost = calculateServicesCost(selectedServiceIds, allServices);
    return (nights * roomPricePerNight) + servicesCost;
}

QString BookingCalculator::determineBookingStatus(const QDate &checkIn, const QDate &today) {
    return (checkIn <= today) ? QStringLiteral("Проживает") : QStringLiteral("Подтверждено");
}

GuestStats BookingCalculator::calculateGuestStats(const QList<Booking> &history) {
    GuestStats stats;
    stats.totalBookings = history.size();
    for (const auto &b : history) {
        if (b.status != QStringLiteral("Отменено")) {
            stats.activeOrCompletedVisits++;
            stats.totalSpent += b.totalCost;
        }
    }
    return stats;
}

QMap<QString, double> BookingCalculator::calculateRevenueByCategory(const QList<Room> &rooms,
                                                                    const QList<Booking> &bookings) {
    QMap<int, QString> roomCategoryMap;
    QMap<QString, double> revenue;

    for (const auto &r : rooms) {
        roomCategoryMap.insert(r.id, r.type);
        if (!revenue.contains(r.type)) {
            revenue.insert(r.type, 0.0);
        }
    }

    for (const auto &b : bookings) {
        if (b.status != QStringLiteral("Отменено")) {
            QString category = roomCategoryMap.value(b.roomId, QStringLiteral("Прочее"));
            revenue[category] += b.totalCost;
        }
    }
    return revenue;
}

double BookingCalculator::convertCurrency(double amountRub, double rate) {
    if (rate <= 0.0) {
        return 0.0;
    }
    return amountRub / rate;
}