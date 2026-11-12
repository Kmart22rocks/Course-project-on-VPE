#ifndef ANALYTICSWIDGET_H
#define ANALYTICSWIDGET_H

#include <QWidget>
#include <QMap>
#include <QLabel>
#include <QPushButton>

class AnalyticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit AnalyticsWidget(QWidget *parent = nullptr);

    void updateAnalyticsData(int totalRooms,
                             int activeBookings,
                             int totalGuests,
                             const QMap<QString, double> &categoryRevenue);
    void updateCurrencyDisplay(double usdRate, double eurRate, bool fromNetwork);

    signals:
        void refreshRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_totalRooms = 0;
    int m_activeBookings = 0;
    int m_totalGuests = 0;
    QMap<QString, double> m_categoryRevenue;

    QLabel *m_kpiLabel;
    QLabel *m_currencyLabel;
    QPushButton *m_refreshBtn;
};

#endif // ANALYTICSWIDGET_H