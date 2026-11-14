#include "bookingcalculator.h"
#include <QtMath>

int BookingCalculator::calculateNights(const QDate &checkIn, const QDate &checkOut) {
    if (!checkIn.isValid() || !checkOut.isValid()) {
        return 0;
    }
    int days = checkIn.daysTo(checkOut);
    return days > 0 ? days : 0;
}

double BookingCalculator::calculateServicesCost(const QList<int> &selectedServiceIds,
                                                const QList<Service> &allServices) {
    double servicesTotal = 0.0;
    for (int id : selectedServiceIds) {
        for (const auto &s : allServices) {
            if (s.id == id) {
                servicesTotal += s.price;
                break;
            }
        }
    }
    return servicesTotal;
}

double BookingCalculator::calculateTotalCost(const QDate &checkIn,
                                             const QDate &checkOut,
                                             double roomPricePerNight,
                                             const QList<int> &selectedServiceIds,
                                             const QList<Service> &allServices) {
    int nights = calculateNights(checkIn, checkOut);
    if (nights <= 0 || roomPricePerNight < 0.0) {
        return 0.0;
    }
    return nights * roomPricePerNight + calculateServicesCost(selectedServiceIds, allServices);
}

QString BookingCalculator::determineBookingStatus(const QDate &checkIn, const QDate &today) {
    if (!checkIn.isValid() || !today.isValid()) {
        return "Забронировано";
    }
    return (checkIn <= today) ? "Активно" : "Забронировано";
}

bool BookingCalculator::isRoomAvailableForDates(int roomId,
                                                const QString &roomStatus,
                                                const QDate &checkIn,
                                                const QDate &checkOut,
                                                const QList<Booking> &bookings,
                                                int ignoreBookingId) {
    if (roomStatus == "На ремонте" || !checkIn.isValid() || !checkOut.isValid() || checkIn >= checkOut) {
        return false;
    }

    for (const Booking &b : bookings) {
        if (b.id == ignoreBookingId) {
            continue;
        }
        if (b.roomId == roomId && b.status != "Отменено" && b.status != "Завершено") {
            if (b.checkIn < checkOut && b.checkOut > checkIn) {
                return false;
            }
        }
    }
    return true;
}

GuestStats BookingCalculator::calculateGuestStats(const QList<Booking> &history) {
    GuestStats stats;
    stats.totalBookings = history.size();
    for (const auto &b : history) {
        if (b.status != "Отменено") {
            stats.totalSpent += b.totalCost;
        }
        if (b.status == "Завершено" || b.status == "Активно") {
            stats.activeOrCompletedVisits++;
        }
    }
    return stats;
}

QMap<QString, double> BookingCalculator::calculateRevenueByCategory(const QList<Room> &rooms,
                                                                    const QList<Booking> &bookings) {
    QMap<int, QString> roomIdToType;
    QMap<QString, double> revenueByType;

    for (const auto &r : rooms) {
        roomIdToType[r.id] = r.type;
        if (!revenueByType.contains(r.type)) {
            revenueByType[r.type] = 0.0;
        }
    }

    for (const auto &b : bookings) {
        if (b.status == "Отменено") {
            continue;
        }
        QString type = roomIdToType.value(b.roomId, "Прочее");
        revenueByType[type] += b.totalCost;
    }

    return revenueByType;
}

double BookingCalculator::convertCurrency(double amountRub, double rate) {
    if (rate <= 0.0001 || amountRub <= 0.0) {
        return 0.0;
    }
    return qRound((amountRub / rate) * 100.0) / 100.0;
}