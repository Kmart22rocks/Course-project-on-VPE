#ifndef ANALYTICSWIDGET_H
#define ANALYTICSWIDGET_H

#include <QWidget>
#include <QMap>
#include <QString>

class QLabel;
class QTabBar;

class AnalyticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit AnalyticsWidget(QWidget *parent = nullptr);

public slots:
    void updateSummaryData(int totalRooms,
                           int activeBookings,
                           int totalGuests,
                           const QMap<QString, double> &categoryRevenue);
    void updateCurrencyRates(double usdRate, double eurRate, bool fromNetwork = true);

    void updateAnalyticsData(int totalRooms,
                             int activeBookings,
                             int totalGuests,
                             const QMap<QString, double> &categoryRevenue) {
        updateSummaryData(totalRooms, activeBookings, totalGuests, categoryRevenue);
    }

    void updateCurrencyDisplay(double usdRate, double eurRate, bool fromNetwork = true) {
        updateCurrencyRates(usdRate, eurRate, fromNetwork);
    }

    signals:
        void refreshRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void loadRoomRevenueFromDb();

    QLabel *m_kpiLabel = nullptr;
    QLabel *m_ratesLabel = nullptr;
    QTabBar *m_chartModeBar = nullptr;
    int m_totalRooms = 0;
    int m_activeBookings = 0;
    int m_totalGuests = 0;
    double m_usdRate = 90.0;
    double m_eurRate = 98.0;
    bool m_ratesFromNetwork = false;
    QMap<QString, double> m_categoryRevenue;
    QMap<QString, double> m_roomRevenue;
};

#endif