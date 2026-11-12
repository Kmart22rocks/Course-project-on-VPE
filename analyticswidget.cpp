#include "analyticswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>

AnalyticsWidget::AnalyticsWidget(QWidget *parent) : QWidget(parent) {
    setStyleSheet("font-size: 11.5pt;");
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
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect chartRect = rect().adjusted(20, 65, -20, -20);
    if (chartRect.height() < 100 || chartRect.width() < 200) {
        return;
    }

    p.setPen(QPen(QColor("#52565e"), 1));
    p.setBrush(QColor("#3b3e45"));
    p.drawRoundedRect(chartRect, 8, 8);

    p.setPen(QColor("#e8eaed"));
    QFont f = font();
    f.setPointSize(13);
    f.setBold(true);
    p.setFont(f);
    p.drawText(chartRect.adjusted(0, 12, 0, 0),
               Qt::AlignTop | Qt::AlignHCenter,
               "Графическая визуализация: Выручка гостиницы по категориям номеров (руб.)");

    if (m_categoryRevenue.isEmpty()) {
        p.drawText(chartRect, Qt::AlignCenter, "Нет данных для построения диаграммы");
        return;
    }

    double maxVal = 0.0;
    for (double v : m_categoryRevenue) {
        if (v > maxVal) maxVal = v;
    }
    maxVal = (maxVal <= 0.0) ? 10000.0 : qCeil(maxVal / 5000.0) * 5000.0;

    QRect plotArea(chartRect.left() + 85,
                   chartRect.top() + 55,
                   chartRect.width() - 115,
                   chartRect.height() - 105);

    p.setBrush(QColor("#43474f"));
    p.drawRect(plotArea);

    f.setPointSize(11);
    p.setFont(f);

    const int gridSteps = 5;
    for (int i = 0; i <= gridSteps; ++i) {
        double ratio = static_cast<double>(i) / gridSteps;
        int y = plotArea.bottom() - qRound(ratio * plotArea.height());

        p.setPen(QPen(QColor("#585d66"), 1, Qt::DashLine));
        p.drawLine(plotArea.left(), y, plotArea.right(), y);

        p.setPen(QColor("#dcdfe4"));
        QRect labelRect(chartRect.left() + 5, y - 12, 72, 24);
        p.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(ratio * maxVal, 'f', 0));
    }

    p.setPen(QPen(QColor("#9aa0a6"), 2));
    p.drawLine(plotArea.topLeft(), plotArea.bottomLeft());
    p.drawLine(plotArea.bottomLeft(), plotArea.bottomRight());

    int count = m_categoryRevenue.size();
    int stepWidth = plotArea.width() / qMax(1, count);
    int barWidth = qMin(85, qMax(30, stepWidth / 2));

    QList<QColor> barColors = {
        QColor("#5c8bc0"),
        QColor("#5fa876"),
        QColor("#d49a45"),
        QColor("#9b6fb6"),
        QColor("#4fa3a5")
    };

    int idx = 0;
    for (auto it = m_categoryRevenue.constBegin(); it != m_categoryRevenue.constEnd(); ++it, ++idx) {
        int barHeight = qRound((it.value() / maxVal) * (plotArea.height() - 25));
        int centerX = plotArea.left() + idx * stepWidth + stepWidth / 2;
        QRect barRect(centerX - barWidth / 2, plotArea.bottom() - barHeight, barWidth, barHeight);

        p.setPen(Qt::NoPen);
        p.setBrush(barColors[idx % barColors.size()]);
        p.drawRoundedRect(barRect, 4, 4);

        p.setPen(QColor("#f1f3f4"));
        p.drawText(QRect(centerX - stepWidth / 2, barRect.top() - 26, stepWidth, 22),
                   Qt::AlignCenter, QString("%1 руб.").arg(it.value(), 0, 'f', 0));

        p.drawText(QRect(centerX - stepWidth / 2, plotArea.bottom() + 8, stepWidth, 32),
                   Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, it.key());
    }
}