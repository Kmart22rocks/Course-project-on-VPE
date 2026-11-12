#include "analyticswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>

AnalyticsWidget::AnalyticsWidget(QWidget *parent) : QWidget(parent) {
    auto *mainLayout = new QVBoxLayout(this);

    auto *topPanel = new QHBoxLayout();
    m_kpiLabel = new QLabel("Загрузка данных с TCP-сервера...", this);
    QFont f = m_kpiLabel->font();
    f.setBold(true);
    f.setPointSize(10);
    m_kpiLabel->setFont(f);

    m_currencyLabel = new QLabel("Курсы валют (ЦБ РФ): загрузка...", this);
    m_refreshBtn = new QPushButton("Обновить по TCP и HTTP API", this);

    topPanel->addWidget(m_kpiLabel);
    topPanel->addStretch();
    topPanel->addWidget(m_currencyLabel);
    topPanel->addWidget(m_refreshBtn);

    mainLayout->addLayout(topPanel);
    mainLayout->addStretch();

    connect(m_refreshBtn, &QPushButton::clicked, this, &AnalyticsWidget::refreshRequested);
}

void AnalyticsWidget::updateAnalyticsData(int totalRooms,
                                          int activeBookings,
                                          int totalGuests,
                                          const QMap<QString, double> &categoryRevenue) {
    m_totalRooms = totalRooms;
    m_activeBookings = activeBookings;
    m_totalGuests = totalGuests;
    m_categoryRevenue = categoryRevenue;

    m_kpiLabel->setText(
        QString("TCP-Сервер: Номеров: %1 | Активных броней: %2 | Гостей в базе: %3")
            .arg(m_totalRooms).arg(m_activeBookings).arg(m_totalGuests)
    );
    update();
}

void AnalyticsWidget::updateCurrencyDisplay(double usdRate, double eurRate, bool fromNetwork) {
    QString src = fromNetwork ? "HTTP JSON API ЦБ РФ" : "Резервный курс";
    m_currencyLabel->setText(
        QString("USD: %1 руб. | EUR: %2 руб. (%3)")
            .arg(usdRate, 0, 'f', 2)
            .arg(eurRate, 0, 'f', 2)
            .arg(src)
    );
}

void AnalyticsWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int topMargin = 60;
    int bottomMargin = 50;
    int leftMargin = 70;
    int rightMargin = 40;

    QRect chartRect(leftMargin, topMargin,
                    width() - leftMargin - rightMargin,
                    height() - topMargin - bottomMargin);

    if (chartRect.width() <= 50 || chartRect.height() <= 50) return;

    painter.fillRect(chartRect, QColor(248, 249, 250));
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    painter.drawRect(chartRect);

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    painter.drawText(QRect(leftMargin, topMargin - 28, chartRect.width(), 24),
                     Qt::AlignCenter,
                     "Визуализация выручки гостиницы по категориям номеров (руб.)");

    if (m_categoryRevenue.isEmpty()) return;

    double maxRev = 1000.0;
    for (double val : m_categoryRevenue) {
        if (val > maxRev) maxRev = val;
    }
    maxRev *= 1.15;

    int count = m_categoryRevenue.size();
    int barSpacing = 30;
    int totalSpacing = barSpacing * (count + 1);
    int barWidth = qMax(30, (chartRect.width() - totalSpacing) / count);

    QList<QColor> colors = {
        QColor(52, 152, 219),
        QColor(46, 204, 113),
        QColor(155, 89, 182),
        QColor(230, 126, 34)
    };

    int idx = 0;
    QFont labelFont = font();
    labelFont.setPointSize(9);
    painter.setFont(labelFont);

    for (auto it = m_categoryRevenue.constBegin(); it != m_categoryRevenue.constEnd(); ++it, ++idx) {
        double value = it.value();
        int barHeight = static_cast<int>((value / maxRev) * (chartRect.height() - 20));
        int x = chartRect.left() + barSpacing + idx * (barWidth + barSpacing);
        int y = chartRect.bottom() - barHeight;

        QRect barRect(x, y, barWidth, barHeight);
        painter.setBrush(colors[idx % colors.size()]);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(barRect, 4, 4);

        painter.setPen(Qt::black);
        painter.drawText(QRect(x - 10, y - 22, barWidth + 20, 20),
                         Qt::AlignCenter,
                         QString("%1 р.").arg(value, 0, 'f', 0));

        painter.drawText(QRect(x - 10, chartRect.bottom() + 6, barWidth + 20, 25),
                         Qt::AlignCenter,
                         it.key());
    }
}