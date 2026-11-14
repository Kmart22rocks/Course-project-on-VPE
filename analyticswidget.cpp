#include "analyticswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabBar>
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>
#include <QSqlQuery>

AnalyticsWidget::AnalyticsWidget(QWidget *parent) : QWidget(parent) {
    setStyleSheet("font-size: 11.5pt;");

    auto *mainLayout = new QVBoxLayout(this);
    auto *topPanel = new QHBoxLayout();

    m_kpiLabel = new QLabel("Данные TCP-сервера: ожидание синхронизации...", this);
    m_kpiLabel->setStyleSheet("font-size: 11.5pt; font-weight: bold;");

    m_ratesLabel = new QLabel("Курсы валют ЦБ РФ (HTTP JSON API): загрузка...", this);
    m_ratesLabel->setStyleSheet("font-size: 11.5pt; color: #8ad4a4; font-weight: bold;");

    topPanel->addWidget(m_kpiLabel);
    topPanel->addStretch();
    topPanel->addWidget(m_ratesLabel);

    auto *switchLayout = new QHBoxLayout();
    m_chartModeBar = new QTabBar(this);
    m_chartModeBar->addTab("Выручка по категориям номеров");
    m_chartModeBar->addTab("Выручка по номерам");
    m_chartModeBar->setExpanding(false);
    connect(m_chartModeBar, &QTabBar::currentChanged, this, [this]() {
        loadRoomRevenueFromDb();
        update();
    });

    auto *refreshBtn = new QPushButton("Запросить сводку по TCP и обновить курсы API", this);
    connect(refreshBtn, &QPushButton::clicked, this, &AnalyticsWidget::refreshRequested);

    switchLayout->addWidget(m_chartModeBar);
    switchLayout->addStretch();
    switchLayout->addWidget(refreshBtn);

    mainLayout->addLayout(topPanel);
    mainLayout->addLayout(switchLayout);
    mainLayout->addStretch(1);
}

void AnalyticsWidget::loadRoomRevenueFromDb() {
    m_roomRevenue.clear();
    QSqlQuery q("SELECT r.room_number, COALESCE(SUM(CASE WHEN b.status != 'Отменено' THEN b.total_cost ELSE 0 END), 0) "
                "FROM rooms r LEFT JOIN bookings b ON r.id = b.room_id "
                "GROUP BY r.id, r.room_number ORDER BY r.room_number ASC");
    while (q.next()) {
        QString key = QString("№%1").arg(q.value(0).toInt(), 3, 10, QChar(' ')).trimmed();
        m_roomRevenue.insert(key, q.value(1).toDouble());
    }
}

void AnalyticsWidget::updateSummaryData(int totalRooms,
                                        int activeBookings,
                                        int totalGuests,
                                        const QMap<QString, double> &categoryRevenue) {
    m_totalRooms = totalRooms;
    m_activeBookings = activeBookings;
    m_totalGuests = totalGuests;
    m_categoryRevenue = categoryRevenue;
    loadRoomRevenueFromDb();

    m_kpiLabel->setText(
        QString("TCP-сервер | Всего номеров: %1 | Активных броней: %2 | Гостей в БД: %3")
            .arg(m_totalRooms)
            .arg(m_activeBookings)
            .arg(m_totalGuests));
    update();
}

void AnalyticsWidget::updateCurrencyRates(double usdRate, double eurRate, bool fromNetwork) {
    m_usdRate = usdRate;
    m_eurRate = eurRate;
    m_ratesFromNetwork = fromNetwork;

    QString sourceText = fromNetwork ? "онлайн ЦБ РФ" : "резервный курс";
    m_ratesLabel->setText(
        QString("HTTP API (%1): 1 USD = %2 руб. | 1 EUR = %3 руб.")
            .arg(sourceText)
            .arg(m_usdRate, 0, 'f', 2)
            .arg(m_eurRate, 0, 'f', 2));
    update();
}

void AnalyticsWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect chartRect = rect().adjusted(20, 100, -20, -20);
    if (chartRect.height() < 100 || chartRect.width() < 200) {
        return;
    }

    p.setPen(QPen(QColor("#52565e"), 1));
    p.setBrush(QColor("#3b3e45"));
    p.drawRoundedRect(chartRect, 8, 8);

    bool byRoom = (m_chartModeBar && m_chartModeBar->currentIndex() == 1);
    const QMap<QString, double> &activeData = byRoom ? m_roomRevenue : m_categoryRevenue;
    QString chartTitle = byRoom
        ? "Графическая визуализация: Выручка гостиницы по номерам (руб.)"
        : "Графическая визуализация: Выручка гостиницы по категориям номеров (руб.)";

    p.setPen(QColor("#e8eaed"));
    QFont f = font();
    f.setPointSize(13);
    f.setBold(true);
    p.setFont(f);
    p.drawText(chartRect.adjusted(0, 12, 0, 0), Qt::AlignTop | Qt::AlignHCenter, chartTitle);

    if (activeData.isEmpty()) {
        p.drawText(chartRect, Qt::AlignCenter, "Нет данных для построения диаграммы");
        return;
    }

    double maxVal = 0.0;
    for (double v : activeData) {
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

    int count = activeData.size();
    int stepWidth = plotArea.width() / qMax(1, count);
    int barWidth = qMin(85, qMax(24, stepWidth / 2));

    QList<QColor> barColors = {
        QColor("#5c8bc0"),
        QColor("#5fa876"),
        QColor("#d49a45"),
        QColor("#9b6fb6"),
        QColor("#4fa3a5")
    };

    int idx = 0;
    for (auto it = activeData.constBegin(); it != activeData.constEnd(); ++it, ++idx) {
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