#include "bookingdialog.h"
#include "bookingcalculator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>

BookingDialog::BookingDialog(DBManager *db,
                             CurrencyApiClient *apiClient,
                             QWidget *parent,
                             int bookingIdToEdit)
    : QDialog(parent),
      m_db(db),
      m_apiClient(apiClient),
      m_bookingIdToEdit(bookingIdToEdit) {
    setupUI();
    loadData();
    if (m_bookingIdToEdit > 0) {
        setWindowTitle("Редактирование бронирования");
    } else {
        setWindowTitle("Новое бронирование");
    }

    if (m_apiClient) {
        connect(m_apiClient, &CurrencyApiClient::ratesUpdated, this, [this]() {
            calculateTotal();
        });
    }
}

void BookingDialog::setupUI() {
    setMinimumWidth(480);
    auto *mainLayout = new QVBoxLayout(this);
    auto *formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit(this);
    m_passportEdit = new QLineEdit(this);
    m_phoneEdit = new QLineEdit(this);

    formLayout->addRow("ФИО гостя:", m_nameEdit);
    formLayout->addRow("Паспорт:", m_passportEdit);
    formLayout->addRow("Телефон:", m_phoneEdit);

    m_checkInEdit = new QDateEdit(QDate::currentDate(), this);
    m_checkInEdit->setCalendarPopup(true);
    m_checkInEdit->setMinimumDate(QDate::currentDate());

    m_checkOutEdit = new QDateEdit(QDate::currentDate().addDays(1), this);
    m_checkOutEdit->setCalendarPopup(true);
    m_checkOutEdit->setMinimumDate(QDate::currentDate().addDays(1));

    formLayout->addRow("Дата заезда:", m_checkInEdit);
    formLayout->addRow("Дата выезда:", m_checkOutEdit);

    m_roomCombo = new QComboBox(this);
    formLayout->addRow("Номер:", m_roomCombo);

    m_servicesList = new QListWidget(this);
    m_servicesList->setMaximumHeight(120);
    formLayout->addRow("Услуги:", m_servicesList);

    m_totalLabel = new QLabel("Итого: 0.00 руб.", this);
    QFont font = m_totalLabel->font();
    font.setBold(true);
    font.setPointSize(10);
    m_totalLabel->setFont(font);
    formLayout->addRow("Стоимость:", m_totalLabel);

    mainLayout->addLayout(formLayout);

    auto *btnLayout = new QHBoxLayout();
    auto *saveBtn = new QPushButton("Сохранить", this);
    auto *cancelBtn = new QPushButton("Отмена", this);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(saveBtn, &QPushButton::clicked, this, &BookingDialog::validateAndAccept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    connect(m_checkInEdit, &QDateEdit::dateChanged, this, [this](const QDate &d){
        m_checkOutEdit->setMinimumDate(d.addDays(1));
        updateAvailableRooms();
        calculateTotal();
    });
    connect(m_checkOutEdit, &QDateEdit::dateChanged, this, [this](){
        updateAvailableRooms();
        calculateTotal();
    });
    connect(m_roomCombo, &QComboBox::currentIndexChanged,
            this, &BookingDialog::calculateTotal);
    connect(m_servicesList, &QListWidget::itemChanged,
            this, &BookingDialog::calculateTotal);
}

void BookingDialog::loadData() {
    m_allServices = m_db->getAllServices();
    for (const auto &s : m_allServices) {
        auto *item = new QListWidgetItem(
            QString("%1 (%2 руб.)").arg(s.name).arg(s.price, 0, 'f', 2), m_servicesList
        );
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, s.id);
    }

    if (m_bookingIdToEdit > 0) {
        m_currentBooking = m_db->getBookingById(m_bookingIdToEdit);
        QList<Guest> guests = m_db->getAllGuests();
        for (const auto &g : guests) {
            if (g.id == m_currentBooking.guestId) {
                m_nameEdit->setText(g.fullName);
                m_passportEdit->setText(g.passport);
                m_phoneEdit->setText(g.phone);
                break;
            }
        }
        m_checkInEdit->setDate(m_currentBooking.checkIn);
        m_checkOutEdit->setDate(m_currentBooking.checkOut);

        updateAvailableRooms();

        for (int i = 0; i < m_roomCombo->count(); ++i) {
            if (m_roomCombo->itemData(i).toInt() == m_currentBooking.roomId) {
                m_roomCombo->setCurrentIndex(i);
                break;
            }
        }

        for (int i = 0; i < m_servicesList->count(); ++i) {
            auto *item = m_servicesList->item(i);
            int sid = item->data(Qt::UserRole).toInt();
            if (m_currentBooking.serviceIds.contains(sid)) {
                item->setCheckState(Qt::Checked);
            }
        }
    } else {
        updateAvailableRooms();
    }
    calculateTotal();
}

void BookingDialog::updateAvailableRooms() {
    m_roomCombo->clear();
    m_availableRooms = m_db->getAvailableRoomsForDates(
        m_checkInEdit->date(), m_checkOutEdit->date(), m_bookingIdToEdit
    );
    for (const auto &r : m_availableRooms) {
        m_roomCombo->addItem(
            QString("№%1 (%2) - %3 руб./ночь").arg(r.roomNumber).arg(r.type).arg(r.pricePerNight, 0, 'f', 2),
            r.id
        );
    }
}

QList<int> BookingDialog::getSelectedServiceIds() const {
    QList<int> selected;
    for (int i = 0; i < m_servicesList->count(); ++i) {
        auto *item = m_servicesList->item(i);
        if (item->checkState() == Qt::Checked) {
            selected.append(item->data(Qt::UserRole).toInt());
        }
    }
    return selected;
}

double BookingDialog::getSelectedRoomPrice() const {
    if (m_roomCombo->currentIndex() < 0) return 0.0;
    int roomId = m_roomCombo->currentData().toInt();
    for (const auto &r : m_availableRooms) {
        if (r.id == roomId) {
            return r.pricePerNight;
        }
    }
    return 0.0;
}

void BookingDialog::calculateTotal() {
    if (m_roomCombo->currentIndex() < 0) {
        m_totalLabel->setText("Итого: 0.00 руб.");
        return;
    }

    int nights = BookingCalculator::calculateNights(m_checkInEdit->date(), m_checkOutEdit->date());
    double totalRub = BookingCalculator::calculateTotalCost(
        m_checkInEdit->date(),
        m_checkOutEdit->date(),
        getSelectedRoomPrice(),
        getSelectedServiceIds(),
        m_allServices
    );

    double usdRate = m_apiClient ? m_apiClient->usdRate() : 90.0;
    double eurRate = m_apiClient ? m_apiClient->eurRate() : 98.0;
    double totalUsd = BookingCalculator::convertCurrency(totalRub, usdRate);
    double totalEur = BookingCalculator::convertCurrency(totalRub, eurRate);

    m_totalLabel->setText(
        QString("%1 руб. (Ночей: %2) | ~$%3 | ~€%4")
            .arg(totalRub, 0, 'f', 2)
            .arg(nights)
            .arg(totalUsd, 0, 'f', 2)
            .arg(totalEur, 0, 'f', 2)
    );
}

void BookingDialog::validateAndAccept() {
    if (m_nameEdit->text().trimmed().isEmpty() ||
        m_passportEdit->text().trimmed().isEmpty() ||
        m_phoneEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Заполните все данные о госте!");
        return;
    }

    if (m_roomCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Предупреждение", "Нет доступных номеров на выбранные даты!");
        return;
    }

    accept();
}

Booking BookingDialog::getBookingData() const {
    Booking b = m_currentBooking;
    Guest g;
    g.fullName = m_nameEdit->text().trimmed();
    g.passport = m_passportEdit->text().trimmed();
    g.phone = m_phoneEdit->text().trimmed();

    b.guestId = m_db->addOrGetGuest(g);
    b.guestName = g.fullName;
    b.roomId = m_roomCombo->currentData().toInt();
    b.checkIn = m_checkInEdit->date();
    b.checkOut = m_checkOutEdit->date();
    b.status = BookingCalculator::determineBookingStatus(b.checkIn);
    b.serviceIds = getSelectedServiceIds();
    b.totalCost = BookingCalculator::calculateTotalCost(
        b.checkIn, b.checkOut, getSelectedRoomPrice(), b.serviceIds, m_allServices
    );

    return b;
}