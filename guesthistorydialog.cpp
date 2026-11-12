#include "guesthistorydialog.h"
#include "bookingcalculator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>

GuestHistoryDialog::GuestHistoryDialog(const Guest &guest, DBManager *db, QWidget *parent)
    : QDialog(parent), m_guest(guest), m_db(db) {
    setWindowTitle(QString("История проживаний: %1").arg(m_guest.fullName));
    setMinimumSize(650, 400);
    setupUI();
    loadHistory();
}

void GuestHistoryDialog::setupUI() {
    auto *layout = new QVBoxLayout(this);

    auto *infoLabel = new QLabel(
        QString("<b>Паспорт:</b> %1 &nbsp;&nbsp;&nbsp; <b>Телефон:</b> %2")
        .arg(m_guest.passport, m_guest.phone), this
    );
    layout->addWidget(infoLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"ID", "Номер комнаты", "Заезд", "Выезд", "Стоимость", "Статус"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_table);

    m_summaryLabel = new QLabel("Всего визитов: 0 | Сумма трат: 0.00 руб.", this);
    QFont f = m_summaryLabel->font();
    f.setBold(true);
    m_summaryLabel->setFont(f);
    layout->addWidget(m_summaryLabel);

    auto *closeBtn = new QPushButton("Закрыть", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
}

void GuestHistoryDialog::loadHistory() {
    QList<Booking> list = m_db->getGuestHistory(m_guest.id);
    m_table->setRowCount(list.size());

    for (int i = 0; i < list.size(); ++i) {
        const auto &b = list[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(b.id)));
        m_table->setItem(i, 1, new QTableWidgetItem(QString("№%1").arg(b.roomNumber)));
        m_table->setItem(i, 2, new QTableWidgetItem(b.checkIn.toString("dd.MM.yyyy")));
        m_table->setItem(i, 3, new QTableWidgetItem(b.checkOut.toString("dd.MM.yyyy")));
        m_table->setItem(i, 4, new QTableWidgetItem(QString("%1 руб.").arg(b.totalCost, 0, 'f', 2)));
        m_table->setItem(i, 5, new QTableWidgetItem(b.status));
    }

    GuestStats stats = BookingCalculator::calculateGuestStats(list);
    m_summaryLabel->setText(
        QString("Всего бронирований: %1 (активных/завершенных: %2) | Суммарно потрачено: %3 руб.")
        .arg(stats.totalBookings)
        .arg(stats.activeOrCompletedVisits)
        .arg(stats.totalSpent, 0, 'f', 2)
    );
}