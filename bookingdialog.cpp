#include "bookingdialog.h"
#include "bookingcalculator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QHeaderView>
#include <QSqlQuery>

BookingDialog::BookingDialog(DBManager *db,
                             CurrencyApiClient *apiClient,
                             QWidget *parent,
                             int editBookingId)
    : QDialog(parent),
      m_db(db),
      m_apiClient(apiClient),
      m_editBookingId(editBookingId) {
    setWindowTitle(m_editBookingId > 0 ? "Редактирование бронирования" : "Оформление бронирования");
    setMinimumWidth(520);
    setupUi();
    loadExistingGuests();
    loadServices();
    if (m_editBookingId > 0) {
        loadBookingForEdit();
    }
    updateAvailableRooms();
}

void BookingDialog::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);

    auto *guestGroup = new QGroupBox("Данные гостя", this);
    auto *guestGroupLayout = new QVBoxLayout(guestGroup);
    auto *guestForm = new QFormLayout();

    m_existingGuestCombo = new QComboBox(this);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Иванов Иван Иванович");

    m_passportEdit = new QLineEdit(this);
    m_passportEdit->setPlaceholderText("1234 567890");

    m_phoneEdit = new QLineEdit(this);
    m_phoneEdit->setPlaceholderText("+7 (900) 000-00-00");

    guestForm->addRow("Выбрать гостя из БД:", m_existingGuestCombo);
    guestForm->addRow("ФИО гостя:", m_nameEdit);
    guestForm->addRow("Паспорт:", m_passportEdit);
    guestForm->addRow("Телефон:", m_phoneEdit);
    guestGroupLayout->addLayout(guestForm);

    m_guestHistoryLabel = new QLabel("Статус: новый гость (ранее не бронировал)", this);
    m_guestHistoryLabel->setWordWrap(true);
    m_guestHistoryLabel->setStyleSheet("color: #9aa0ac; font-style: italic; margin-top: 4px;");
    guestGroupLayout->addWidget(m_guestHistoryLabel);

    m_historyBtn = new QPushButton("Посмотреть полную историю проживаний гостя", this);
    m_historyBtn->setEnabled(false);
    guestGroupLayout->addWidget(m_historyBtn);

    mainLayout->addWidget(guestGroup);

    auto *stayGroup = new QGroupBox("Параметры размещения", this);
    auto *stayForm = new QFormLayout(stayGroup);

    m_checkInEdit = new QDateEdit(QDate::currentDate(), this);
    m_checkInEdit->setCalendarPopup(true);
    m_checkInEdit->setDisplayFormat("dd.MM.yyyy");

    m_checkOutEdit = new QDateEdit(QDate::currentDate().addDays(1), this);
    m_checkOutEdit->setCalendarPopup(true);
    m_checkOutEdit->setDisplayFormat("dd.MM.yyyy");

    m_roomCombo = new QComboBox(this);

    stayForm->addRow("Дата заезда:", m_checkInEdit);
    stayForm->addRow("Дата выезда:", m_checkOutEdit);
    stayForm->addRow("Доступный номер:", m_roomCombo);
    mainLayout->addWidget(stayGroup);

    auto *servicesGroup = new QGroupBox("Дополнительные услуги", this);
    auto *servicesLayout = new QVBoxLayout(servicesGroup);
    m_servicesList = new QListWidget(this);
    m_servicesList->setMaximumHeight(130);
    servicesLayout->addWidget(m_servicesList);
    mainLayout->addWidget(servicesGroup);

    m_totalLabel = new QLabel("Итого к оплате: 0.00 руб.", this);
    m_totalLabel->setStyleSheet("font-size: 13pt; font-weight: bold; color: #8ad4a4; margin: 6px 0;");
    m_totalLabel->setWordWrap(true);
    mainLayout->addWidget(m_totalLabel);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto *okBtn = new QPushButton(m_editBookingId > 0 ? "Сохранить изменения" : "Подтвердить бронирование", this);
    auto *cancelBtn = new QPushButton("Отмена", this);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_existingGuestCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookingDialog::onExistingGuestSelected);
    connect(m_passportEdit, &QLineEdit::textEdited,
            this, &BookingDialog::onPassportEdited);
    connect(m_historyBtn, &QPushButton::clicked,
            this, &BookingDialog::onShowGuestHistory);

    connect(m_checkInEdit, &QDateEdit::dateChanged, this, &BookingDialog::onDatesChanged);
    connect(m_checkOutEdit, &QDateEdit::dateChanged, this, &BookingDialog::onDatesChanged);
    connect(m_roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BookingDialog::calculateTotal);
    connect(m_servicesList, &QListWidget::itemChanged,
            this, &BookingDialog::calculateTotal);

    connect(okBtn, &QPushButton::clicked, this, &BookingDialog::validateAndAccept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void BookingDialog::loadExistingGuests() {
    QSignalBlocker blocker(m_existingGuestCombo);
    m_existingGuestCombo->clear();
    m_existingGuestCombo->addItem("-- Новый гость (ввести вручную) --", -1);

    if (!m_db) return;
    m_guests = m_db->getAllGuests();
    for (const Guest &g : m_guests) {
        QString itemText = QString("%1 (паспорт: %2)").arg(g.fullName, g.passport);
        m_existingGuestCombo->addItem(itemText, g.id);
    }
}

void BookingDialog::loadBookingForEdit() {
    if (!m_db || m_editBookingId <= 0) return;

    QList<Booking> bookings = m_db->getAllBookings();
    for (const Booking &b : bookings) {
        if (b.id == m_editBookingId) {
            m_targetRoomId = b.roomId;

            int guestIdx = m_existingGuestCombo->findData(b.guestId);
            if (guestIdx >= 0) {
                m_existingGuestCombo->setCurrentIndex(guestIdx);
            }

            {
                QSignalBlocker b1(m_checkInEdit);
                QSignalBlocker b2(m_checkOutEdit);
                m_checkInEdit->setDate(b.checkIn);
                m_checkOutEdit->setDate(b.checkOut);
            }

            QList<int> sIds = b.serviceIds;
            if (sIds.isEmpty()) {
                QSqlQuery q;
                q.prepare("SELECT service_id FROM booking_services WHERE booking_id = :bid");
                q.bindValue(":bid", m_editBookingId);
                if (q.exec()) {
                    while (q.next()) {
                        sIds.append(q.value(0).toInt());
                    }
                }
            }

            QSignalBlocker b3(m_servicesList);
            for (int i = 0; i < m_servicesList->count(); ++i) {
                auto *item = m_servicesList->item(i);
                int sid = item->data(Qt::UserRole).toInt();
                item->setCheckState(sIds.contains(sid) ? Qt::Checked : Qt::Unchecked);
            }
            break;
        }
    }
}

void BookingDialog::onExistingGuestSelected(int index) {
    if (index < 0) return;

    int guestId = m_existingGuestCombo->itemData(index).toInt();
    if (guestId <= 0) {
        QSignalBlocker passBlocker(m_passportEdit);
        m_nameEdit->clear();
        m_passportEdit->clear();
        m_phoneEdit->clear();
        updateGuestHistoryPreview(-1);
        return;
    }

    for (const Guest &g : m_guests) {
        if (g.id == guestId) {
            QSignalBlocker passBlocker(m_passportEdit);
            m_nameEdit->setText(g.fullName);
            m_passportEdit->setText(g.passport);
            m_phoneEdit->setText(g.phone);
            updateGuestHistoryPreview(g.id);
            break;
        }
    }
}

void BookingDialog::onPassportEdited(const QString &passportText) {
    QString trimmed = passportText.trimmed();
    if (trimmed.isEmpty()) {
        QSignalBlocker comboBlocker(m_existingGuestCombo);
        m_existingGuestCombo->setCurrentIndex(0);
        updateGuestHistoryPreview(-1);
        return;
    }

    for (const Guest &g : m_guests) {
        if (g.passport.compare(trimmed, Qt::CaseInsensitive) == 0) {
            QSignalBlocker comboBlocker(m_existingGuestCombo);
            int idx = m_existingGuestCombo->findData(g.id);
            if (idx >= 0) {
                m_existingGuestCombo->setCurrentIndex(idx);
            }
            m_nameEdit->setText(g.fullName);
            m_phoneEdit->setText(g.phone);
            updateGuestHistoryPreview(g.id);
            return;
        }
    }

    QSignalBlocker comboBlocker(m_existingGuestCombo);
    m_existingGuestCombo->setCurrentIndex(0);
    updateGuestHistoryPreview(-1);
}

void BookingDialog::updateGuestHistoryPreview(int guestId) {
    m_selectedGuestId = guestId;

    if (guestId <= 0 || !m_db) {
        m_guestHistoryLabel->setText("Статус: новый гость (ранее не бронировал)");
        m_guestHistoryLabel->setStyleSheet("color: #9aa0ac; font-style: italic; margin-top: 4px;");
        m_historyBtn->setEnabled(false);
        return;
    }

    QList<Booking> history = m_db->getGuestHistory(guestId);
    GuestStats stats = BookingCalculator::calculateGuestStats(history);

    if (history.isEmpty()) {
        m_guestHistoryLabel->setText("Гость зарегистрирован в базе, но прошлых бронирований нет.");
    } else {
        const Booking &last = history.first();
        QString text = QString(
            "История гостя: всего броней — %1 (проживаний: %2), на сумму — %3 руб.\n"
            "Последний визит: комната №%4 (%5 — %6, статус: %7)")
            .arg(stats.totalBookings)
            .arg(stats.activeOrCompletedVisits)
            .arg(stats.totalSpent, 0, 'f', 2)
            .arg(last.roomNumber)
            .arg(last.checkIn.toString("dd.MM.yyyy"))
            .arg(last.checkOut.toString("dd.MM.yyyy"))
            .arg(last.status);
        m_guestHistoryLabel->setText(text);
    }

    m_guestHistoryLabel->setStyleSheet("color: #8ad4a4; font-weight: bold; margin-top: 4px;");
    m_historyBtn->setEnabled(true);
}

void BookingDialog::onShowGuestHistory() {
    if (m_selectedGuestId <= 0 || !m_db) return;

    QList<Booking> history = m_db->getGuestHistory(m_selectedGuestId);
    GuestStats stats = BookingCalculator::calculateGuestStats(history);

    QDialog dlg(this);
    dlg.setWindowTitle("История бронирований: " + m_nameEdit->text().trimmed());
    dlg.resize(680, 400);

    auto *layout = new QVBoxLayout(&dlg);
    auto *summaryLabel = new QLabel(
        QString("Всего бронирований: %1 | Активных/завершённых визитов: %2 | Общая сумма: %3 руб.")
            .arg(stats.totalBookings)
            .arg(stats.activeOrCompletedVisits)
            .arg(stats.totalSpent, 0, 'f', 2),
        &dlg);
    summaryLabel->setStyleSheet("font-weight: bold; font-size: 10.5pt; margin-bottom: 6px;");
    layout->addWidget(summaryLabel);

    auto *table = new QTableWidget(&dlg);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"ID", "Номер", "Дата заезда", "Дата выезда", "Стоимость (руб.)", "Статус"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setRowCount(history.size());

    for (int i = 0; i < history.size(); ++i) {
        const Booking &b = history[i];
        table->setItem(i, 0, new QTableWidgetItem(QString::number(b.id)));
        table->setItem(i, 1, new QTableWidgetItem(QString("%1").arg(b.roomNumber)));
        table->setItem(i, 2, new QTableWidgetItem(b.checkIn.toString("dd.MM.yyyy")));
        table->setItem(i, 3, new QTableWidgetItem(b.checkOut.toString("dd.MM.yyyy")));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(b.totalCost, 'f', 2)));
        table->setItem(i, 5, new QTableWidgetItem(b.status));
    }
    layout->addWidget(table);

    auto *closeBtn = new QPushButton("Закрыть", &dlg);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(closeBtn);

    dlg.exec();
}

void BookingDialog::loadServices() {
    m_servicesList->clear();
    if (!m_db) return;

    m_allServices = m_db->getAllServices();
    for (const Service &s : m_allServices) {
        auto *item = new QListWidgetItem(
            QString("%1 — %2 руб.").arg(s.name).arg(s.price, 0, 'f', 2),
            m_servicesList);
        item->setData(Qt::UserRole, s.id);
        item->setCheckState(Qt::Unchecked);
    }
}

void BookingDialog::onDatesChanged() {
    if (m_checkOutEdit->date() <= m_checkInEdit->date()) {
        m_checkOutEdit->setDate(m_checkInEdit->date().addDays(1));
    }
    updateAvailableRooms();
}

void BookingDialog::updateAvailableRooms() {
    if (m_roomCombo->count() > 0 && m_roomCombo->currentData().toInt() > 0) {
        m_targetRoomId = m_roomCombo->currentData().toInt();
    }

    m_roomCombo->clear();
    if (!m_db) return;

    QDate checkIn = m_checkInEdit->date();
    QDate checkOut = m_checkOutEdit->date();

    QList<Room> rooms = m_db->getAllRooms();
    QList<Booking> bookings = m_db->getAllBookings();
    int preselectedIdx = -1;

    for (const Room &r : rooms) {
        if (r.status == "На ремонте" && r.id != m_targetRoomId) {
            continue;
        }

        bool available = true;
        for (const Booking &b : bookings) {
            if (b.id == m_editBookingId) {
                continue;
            }
            if (b.roomId == r.id && b.status != "Отменено" && b.status != "Завершено") {
                if (b.checkIn < checkOut && b.checkOut > checkIn) {
                    available = false;
                    break;
                }
            }
        }

        if (available) {
            QString text = QString("№%1 | %2 | %3 руб./сут.")
                               .arg(r.roomNumber)
                               .arg(r.type)
                               .arg(r.pricePerNight, 0, 'f', 0);
            m_roomCombo->addItem(text, r.id);
            int idx = m_roomCombo->count() - 1;
            m_roomCombo->setItemData(idx, r.pricePerNight, Qt::UserRole + 1);
            m_roomCombo->setItemData(idx, r.roomNumber, Qt::UserRole + 2);

            if (r.id == m_targetRoomId) {
                preselectedIdx = idx;
            }
        }
    }

    if (m_roomCombo->count() == 0) {
        m_roomCombo->addItem("Нет свободных номеров на выбранные даты", -1);
    } else if (preselectedIdx >= 0) {
        m_roomCombo->setCurrentIndex(preselectedIdx);
    }

    calculateTotal();
}

void BookingDialog::calculateTotal() {
    int roomId = m_roomCombo->currentData().toInt();
    if (roomId <= 0) {
        m_totalLabel->setText("Итого к оплате: 0.00 руб. (нет свободных номеров)");
        return;
    }

    double pricePerNight = m_roomCombo->currentData(Qt::UserRole + 1).toDouble();
    QList<int> selectedServiceIds;
    for (int i = 0; i < m_servicesList->count(); ++i) {
        auto *item = m_servicesList->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedServiceIds.append(item->data(Qt::UserRole).toInt());
        }
    }

    int nights = BookingCalculator::calculateNights(m_checkInEdit->date(), m_checkOutEdit->date());
    double totalRub = BookingCalculator::calculateTotalCost(
        m_checkInEdit->date(),
        m_checkOutEdit->date(),
        pricePerNight,
        selectedServiceIds,
        m_allServices);

    double usdRate = m_apiClient ? m_apiClient->usdRate() : 90.0;
    double eurRate = m_apiClient ? m_apiClient->eurRate() : 98.0;
    double totalUsd = BookingCalculator::convertCurrency(totalRub, usdRate);
    double totalEur = BookingCalculator::convertCurrency(totalRub, eurRate);

    m_totalLabel->setText(
        QString("Итого за %1 сут.: %2 руб.  (~$%3 / ~€%4)")
            .arg(nights)
            .arg(totalRub, 0, 'f', 2)
            .arg(totalUsd, 0, 'f', 2)
            .arg(totalEur, 0, 'f', 2));
}

void BookingDialog::validateAndAccept() {
    if (m_nameEdit->text().trimmed().isEmpty() ||
        m_passportEdit->text().trimmed().isEmpty() ||
        m_phoneEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, заполните все данные гостя (ФИО, паспорт, телефон).");
        return;
    }

    int roomId = m_roomCombo->currentData().toInt();
    if (roomId <= 0) {
        QMessageBox::warning(this, "Внимание", "На выбранные даты нет доступного номера.");
        return;
    }

    accept();
}

Guest BookingDialog::getGuestData() const {
    Guest g;
    g.id = m_selectedGuestId;
    g.fullName = m_nameEdit->text().trimmed();
    g.passport = m_passportEdit->text().trimmed();
    g.phone = m_phoneEdit->text().trimmed();
    return g;
}

Booking BookingDialog::getBookingData() const {
    Booking b;
    b.id = m_editBookingId;
    b.guestId = m_selectedGuestId;
    b.roomId = m_roomCombo->currentData().toInt();
    b.roomNumber = m_roomCombo->currentData(Qt::UserRole + 2).toInt();
    b.guestName = m_nameEdit->text().trimmed();
    b.checkIn = m_checkInEdit->date();
    b.checkOut = m_checkOutEdit->date();
    b.status = BookingCalculator::determineBookingStatus(b.checkIn);

    double pricePerNight = m_roomCombo->currentData(Qt::UserRole + 1).toDouble();
    for (int i = 0; i < m_servicesList->count(); ++i) {
        auto *item = m_servicesList->item(i);
        if (item->checkState() == Qt::Checked) {
            b.serviceIds.append(item->data(Qt::UserRole).toInt());
        }
    }

    b.totalCost = BookingCalculator::calculateTotalCost(
        b.checkIn,
        b.checkOut,
        pricePerNight,
        b.serviceIds,
        m_allServices);

    return b;
}