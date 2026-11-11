#include "bookingdialog.h"
#include "dbmanager.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

BookingDialog::BookingDialog(QWidget *parent, int bookingIdToEdit)
    : QDialog(parent), m_editBookingId(bookingIdToEdit) {

    if (m_editBookingId > 0) {
        setWindowTitle(QString("Редактирование бронирования #%1").arg(m_editBookingId));
    } else {
        setWindowTitle("Оформление бронирования");
    }

    resize(450, 500);
    setupUI();

    if (m_editBookingId > 0) {
        m_saveBtn->setText("Сохранить изменения");
    }

    m_allServices = DBManager::instance().getAllServices();

    if (m_editBookingId > 0) {
        m_existingBooking = DBManager::instance().getBookingById(m_editBookingId);

        for (const auto &guest : DBManager::instance().getAllGuests()) {
            if (guest.id == m_existingBooking.guestId) {
                m_nameEdit->setText(guest.fullName);
                m_passportEdit->setText(guest.passport);
                m_phoneEdit->setText(guest.phone);
                break;
            }
        }

        m_nameEdit->setReadOnly(true);
        m_passportEdit->setReadOnly(true);
        m_phoneEdit->setReadOnly(true);

        m_checkInEdit->setDate(m_existingBooking.checkIn);
        m_checkOutEdit->setDate(m_existingBooking.checkOut);
    }

    for (const auto &service : m_allServices) {
        auto *item = new QListWidgetItem(
            QString("%1 (%2 руб.)").arg(service.name).arg(service.price),
            m_servicesList
        );
        item->setData(Qt::UserRole, service.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        bool isChecked = (m_editBookingId > 0 && m_existingBooking.serviceIds.contains(service.id));
        item->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
    }

    connect(m_checkInEdit, &QDateEdit::dateChanged, this, &BookingDialog::updateAvailableRooms);
    connect(m_checkOutEdit, &QDateEdit::dateChanged, this, &BookingDialog::updateAvailableRooms);
    connect(m_roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BookingDialog::calculateTotalCost);
    connect(m_servicesList, &QListWidget::itemChanged, this, &BookingDialog::calculateTotalCost);
    connect(m_saveBtn, &QPushButton::clicked, this, &BookingDialog::onSave);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    updateAvailableRooms();
}

void BookingDialog::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    auto *formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit(this);
    m_passportEdit = new QLineEdit(this);
    m_phoneEdit = new QLineEdit(this);

    m_checkInEdit = new QDateEdit(QDate::currentDate(), this);
    m_checkInEdit->setCalendarPopup(true);

    m_checkOutEdit = new QDateEdit(QDate::currentDate().addDays(1), this);
    m_checkOutEdit->setCalendarPopup(true);

    m_roomCombo = new QComboBox(this);
    m_servicesList = new QListWidget(this);
    m_totalCostLabel = new QLabel("<b>Итоговая стоимость: 0 руб.</b>", this);

    formLayout->addRow("ФИО гостя:", m_nameEdit);
    formLayout->addRow("Паспорт:", m_passportEdit);
    formLayout->addRow("Телефон:", m_phoneEdit);
    formLayout->addRow("Дата заезда:", m_checkInEdit);
    formLayout->addRow("Дата выезда:", m_checkOutEdit);
    formLayout->addRow("Свободный номер:", m_roomCombo);
    formLayout->addRow("Доп. услуги:", m_servicesList);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_totalCostLabel);

    auto *btnLayout = new QHBoxLayout();
    m_saveBtn = new QPushButton("Забронировать", this);
    m_cancelBtn = new QPushButton("Отмена", this);

    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);
}

void BookingDialog::updateAvailableRooms() {
    QDate checkIn = m_checkInEdit->date();
    QDate checkOut = m_checkOutEdit->date();

    if (checkOut <= checkIn) {
        m_roomCombo->clear();
        m_availableRooms.clear();
        m_totalCostLabel->setText("<b style='color:red;'>Дата выезда должна быть позже даты заезда!</b>");
        return;
    }

    m_availableRooms = DBManager::instance().getAvailableRoomsForDates(checkIn, checkOut, m_editBookingId);
    m_roomCombo->clear();

    int selectedIndex = -1;
    if (m_availableRooms.isEmpty()) {
        m_roomCombo->addItem("Нет свободных номеров на эти даты");
    } else {
        for (int i = 0; i < m_availableRooms.size(); ++i) {
            const auto &room = m_availableRooms[i];
            m_roomCombo->addItem(
                QString("№%1 (%2) — %3 руб./ночь")
                    .arg(room.roomNumber)
                    .arg(room.type)
                    .arg(room.pricePerNight),
                room.id
            );

            if (m_editBookingId > 0 && room.id == m_existingBooking.roomId) {
                selectedIndex = i;
            }
        }
    }

    if (selectedIndex >= 0) {
        m_roomCombo->setCurrentIndex(selectedIndex);
    }

    calculateTotalCost();
}

void BookingDialog::calculateTotalCost() {
    QDate checkIn = m_checkInEdit->date();
    QDate checkOut = m_checkOutEdit->date();
    int roomIdx = m_roomCombo->currentIndex();

    if (checkOut <= checkIn || m_availableRooms.isEmpty() || roomIdx < 0 || roomIdx >= m_availableRooms.size()) {
        m_totalCostLabel->setText("<b>Итоговая стоимость: 0 руб.</b>");
        return;
    }

    qint64 nights = checkIn.daysTo(checkOut);
    double cost = nights * m_availableRooms[roomIdx].pricePerNight;

    for (int i = 0; i < m_servicesList->count(); ++i) {
        auto *item = m_servicesList->item(i);
        if (item->checkState() == Qt::Checked) {
            int serviceId = item->data(Qt::UserRole).toInt();
            for (const auto &service : m_allServices) {
                if (service.id == serviceId) {
                    cost += service.price;
                    break;
                }
            }
        }
    }

    m_totalCostLabel->setText(
        QString("<b>Итоговая стоимость: %1 руб. (%2 ноч.)</b>")
            .arg(cost)
            .arg(nights)
    );
}

void BookingDialog::onSave() {
    if (m_nameEdit->text().trimmed().isEmpty() ||
        m_passportEdit->text().trimmed().isEmpty() ||
        m_phoneEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все данные гостя!");
        return;
    }

    if (m_checkOutEdit->date() <= m_checkInEdit->date() || m_availableRooms.isEmpty() || m_roomCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Ошибка", "Проверьте корректность дат и выбор доступного номера!");
        return;
    }

    int roomIdx = m_roomCombo->currentIndex();
    int roomId = m_availableRooms[roomIdx].id;
    qint64 nights = m_checkInEdit->date().daysTo(m_checkOutEdit->date());
    double totalCost = nights * m_availableRooms[roomIdx].pricePerNight;

    QList<int> selectedServiceIds;
    for (int i = 0; i < m_servicesList->count(); ++i) {
        auto *item = m_servicesList->item(i);
        if (item->checkState() == Qt::Checked) {
            int serviceId = item->data(Qt::UserRole).toInt();
            selectedServiceIds.append(serviceId);
            for (const auto &service : m_allServices) {
                if (service.id == serviceId) {
                    totalCost += service.price;
                    break;
                }
            }
        }
    }

    if (m_editBookingId > 0) {
        Booking booking;
        booking.id = m_editBookingId;
        booking.guestId = m_existingBooking.guestId;
        booking.roomId = roomId;
        booking.checkIn = m_checkInEdit->date();
        booking.checkOut = m_checkOutEdit->date();
        booking.status = m_existingBooking.status;
        booking.totalCost = totalCost;
        booking.serviceIds = selectedServiceIds;

        if (DBManager::instance().updateBooking(booking)) {
            QMessageBox::information(this, "Успех", "Бронирование успешно обновлено!");
            accept();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось обновить данные бронирования.");
        }
    } else {
        Guest guest;
        guest.fullName = m_nameEdit->text().trimmed();
        guest.passport = m_passportEdit->text().trimmed();
        guest.phone = m_phoneEdit->text().trimmed();

        int guestId = DBManager::instance().addOrGetGuest(guest);
        if (guestId < 0) {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить данные гостя.");
            return;
        }

        Booking booking;
        booking.guestId = guestId;
        booking.roomId = roomId;
        booking.checkIn = m_checkInEdit->date();
        booking.checkOut = m_checkOutEdit->date();
        booking.status = "Активно";
        booking.totalCost = totalCost;
        booking.serviceIds = selectedServiceIds;

        if (DBManager::instance().addBooking(booking)) {
            QMessageBox::information(this, "Успех", "Бронирование успешно создано!");
            accept();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось добавить новое бронирование.");
        }
    }
}