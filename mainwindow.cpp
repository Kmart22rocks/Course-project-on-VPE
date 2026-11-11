#include "mainwindow.h"
#include "dbmanager.h"
#include "bookingdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QIntValidator>

static void addTableRow(QTableWidget *table, const QStringList &values) {
    int row = table->rowCount();
    table->insertRow(row);
    for (int col = 0; col < values.size(); ++col) {
        table->setItem(row, col, new QTableWidgetItem(values[col]));
    }
}

static void configureTable(QTableWidget *table, const QStringList &headers) {
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Система управления гостиничным комплексом");
    resize(900, 600);

    setupUI();

    loadRooms();
    loadBookings();
    loadGuests();
    loadServices();
}

void MainWindow::setupUI() {
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);

    // --- Вкладка 1: Номера ---
    auto *roomsTab = new QWidget();
    auto *roomsLayout = new QVBoxLayout(roomsTab);

    m_roomsTable = new QTableWidget(roomsTab);
    configureTable(m_roomsTable, {"ID", "Номер", "Тип", "Цена за ночь (руб.)", "Статус"});

    auto *roomBtnsLayout = new QHBoxLayout();
    m_addRoomBtn = new QPushButton("Добавить номер", roomsTab);
    m_editRoomBtn = new QPushButton("Изменить номер", roomsTab);
    m_deleteRoomBtn = new QPushButton("Удалить номер", roomsTab);

    connect(m_addRoomBtn, &QPushButton::clicked, this, &MainWindow::addRoomDialog);
    connect(m_editRoomBtn, &QPushButton::clicked, this, &MainWindow::editRoomDialog);
    connect(m_deleteRoomBtn, &QPushButton::clicked, this, &MainWindow::deleteSelectedRoom);

    roomBtnsLayout->addWidget(m_addRoomBtn);
    roomBtnsLayout->addWidget(m_editRoomBtn);
    roomBtnsLayout->addWidget(m_deleteRoomBtn);

    roomsLayout->addWidget(m_roomsTable);
    roomsLayout->addLayout(roomBtnsLayout);
    m_tabWidget->addTab(roomsTab, "Номера");

    // --- Вкладка 2: Бронирования ---
    auto *bookingsTab = new QWidget();
    auto *bookingsLayout = new QVBoxLayout(bookingsTab);

    m_bookingsTable = new QTableWidget(bookingsTab);
    configureTable(m_bookingsTable, {"ID", "Гость", "Номер", "Дата заезда", "Дата выезда", "Стоимость (руб.)", "Статус"});

    auto *bookingBtnsLayout = new QHBoxLayout();
    m_newBookingBtn = new QPushButton("Новое бронирование", bookingsTab);
    m_editBookingBtn = new QPushButton("Изменить бронирование", bookingsTab);
    m_cancelBookingBtn = new QPushButton("Отменить бронирование", bookingsTab);

    connect(m_newBookingBtn, &QPushButton::clicked, this, &MainWindow::openBookingDialog);
    connect(m_editBookingBtn, &QPushButton::clicked, this, &MainWindow::editSelectedBooking);
    connect(m_cancelBookingBtn, &QPushButton::clicked, this, &MainWindow::cancelSelectedBooking);

    bookingBtnsLayout->addWidget(m_newBookingBtn);
    bookingBtnsLayout->addWidget(m_editBookingBtn);
    bookingBtnsLayout->addWidget(m_cancelBookingBtn);

    bookingsLayout->addWidget(m_bookingsTable);
    bookingsLayout->addLayout(bookingBtnsLayout);
    m_tabWidget->addTab(bookingsTab, "Бронирования");

    // --- Вкладка 3: Гости и История ---
    auto *guestsTab = new QWidget();
    auto *guestsLayout = new QVBoxLayout(guestsTab);

    m_guestsTable = new QTableWidget(guestsTab);
    configureTable(m_guestsTable, {"ID", "ФИО", "Паспорт", "Телефон"});

    m_guestHistoryBtn = new QPushButton("Посмотреть историю гостя", guestsTab);
    connect(m_guestHistoryBtn, &QPushButton::clicked, this, &MainWindow::showGuestHistory);

    guestsLayout->addWidget(m_guestsTable);
    guestsLayout->addWidget(m_guestHistoryBtn);
    m_tabWidget->addTab(guestsTab, "Гости и История");

    // --- Вкладка 4: Услуги ---
    auto *servicesTab = new QWidget();
    auto *servicesLayout = new QVBoxLayout(servicesTab);

    m_servicesTable = new QTableWidget(servicesTab);
    configureTable(m_servicesTable, {"ID", "Название", "Цена (руб.)"});

    auto *serviceBtnsLayout = new QHBoxLayout();
    m_addServiceBtn = new QPushButton("Добавить услугу", servicesTab);
    m_editServiceBtn = new QPushButton("Изменить услугу", servicesTab);
    m_deleteServiceBtn = new QPushButton("Удалить услугу", servicesTab);

    connect(m_addServiceBtn, &QPushButton::clicked, this, &MainWindow::addServiceDialog);
    connect(m_editServiceBtn, &QPushButton::clicked, this, &MainWindow::editServiceDialog);
    connect(m_deleteServiceBtn, &QPushButton::clicked, this, &MainWindow::deleteSelectedService);

    serviceBtnsLayout->addWidget(m_addServiceBtn);
    serviceBtnsLayout->addWidget(m_editServiceBtn);
    serviceBtnsLayout->addWidget(m_deleteServiceBtn);

    servicesLayout->addWidget(m_servicesTable);
    servicesLayout->addLayout(serviceBtnsLayout);
    m_tabWidget->addTab(servicesTab, "Услуги");
}

void MainWindow::loadRooms() {
    m_roomsTable->setRowCount(0);
    for (const auto &room : DBManager::instance().getAllRooms()) {
        addTableRow(m_roomsTable, {
            QString::number(room.id),
            QString::number(room.roomNumber),
            room.type,
            QString::number(room.pricePerNight, 'f', 2),
            room.status
        });
    }
}

void MainWindow::loadBookings() {
    m_bookingsTable->setRowCount(0);
    QDate today = QDate::currentDate();

    for (const auto &booking : DBManager::instance().getAllBookings()) {
        QString statusText = (booking.checkIn <= today) ? "Проживает" : "Будущее бронирование";

        addTableRow(m_bookingsTable, {
            QString::number(booking.id),
            booking.guestName,
            QString::number(booking.roomNumber),
            booking.checkIn.toString("dd.MM.yyyy"),
            booking.checkOut.toString("dd.MM.yyyy"),
            QString::number(booking.totalCost, 'f', 2),
            statusText
        });
    }
}

void MainWindow::loadGuests() {
    m_guestsTable->setRowCount(0);
    for (const auto &guest : DBManager::instance().getAllGuests()) {
        addTableRow(m_guestsTable, {
            QString::number(guest.id),
            guest.fullName,
            guest.passport,
            guest.phone
        });
    }
}

void MainWindow::loadServices() {
    m_servicesTable->setRowCount(0);
    for (const auto &service : DBManager::instance().getAllServices()) {
        addTableRow(m_servicesTable, {
            QString::number(service.id),
            service.name,
            QString::number(service.price, 'f', 2)
        });
    }
}

void MainWindow::openBookingDialog() {
    BookingDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        loadBookings();
        loadRooms();
        loadGuests();
    }
}

void MainWindow::editSelectedBooking() {
    int row = m_bookingsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите бронирование для редактирования!");
        return;
    }

    int bookingId = m_bookingsTable->item(row, 0)->text().toInt();
    BookingDialog dlg(this, bookingId);
    if (dlg.exec() == QDialog::Accepted) {
        loadBookings();
        loadRooms();
        loadGuests();
    }
}

void MainWindow::cancelSelectedBooking() {
    int row = m_bookingsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите бронирование для отмены!");
        return;
    }

    int bookingId = m_bookingsTable->item(row, 0)->text().toInt();
    auto reply = QMessageBox::question(
        this,
        "Подтверждение",
        QString("Вы действительно хотите отменить бронирование #%1?").arg(bookingId),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (DBManager::instance().cancelBooking(bookingId)) {
            loadBookings();
            loadRooms();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось отменить бронирование.");
        }
    }
}

void MainWindow::addRoomDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle("Добавить номер");
    QFormLayout layout(&dlg);

    QLineEdit numEdit;
    numEdit.setValidator(new QIntValidator(1, 9999, &dlg));

    QLineEdit typeEdit;
    QLineEdit priceEdit;
    priceEdit.setValidator(new QDoubleValidator(0.0, 1000000.0, 2, &dlg));

    layout.addRow("Номер:", &numEdit);
    layout.addRow("Тип:", &typeEdit);
    layout.addRow("Цена за ночь:", &priceEdit);

    QPushButton saveBtn("Сохранить", &dlg);
    layout.addWidget(&saveBtn);

    connect(&saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted) {
        if (numEdit.text().isEmpty() || typeEdit.text().isEmpty() || priceEdit.text().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Заполните все поля!");
            return;
        }

        Room room;
        room.roomNumber = numEdit.text().toInt();
        room.type = typeEdit.text().trimmed();
        room.pricePerNight = priceEdit.text().toDouble();
        room.status = "Свободен";

        if (DBManager::instance().addRoom(room)) {
            loadRooms();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось добавить номер (возможно, такой номер уже существует).");
        }
    }
}

void MainWindow::editRoomDialog() {
    int row = m_roomsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите номер для редактирования!");
        return;
    }

    int roomId = m_roomsTable->item(row, 0)->text().toInt();

    QDialog dlg(this);
    dlg.setWindowTitle("Изменить номер");
    QFormLayout layout(&dlg);

    QLineEdit numEdit(m_roomsTable->item(row, 1)->text());
    numEdit.setValidator(new QIntValidator(1, 9999, &dlg));

    QLineEdit typeEdit(m_roomsTable->item(row, 2)->text());
    QLineEdit priceEdit(m_roomsTable->item(row, 3)->text());
    priceEdit.setValidator(new QDoubleValidator(0.0, 1000000.0, 2, &dlg));

    layout.addRow("Номер:", &numEdit);
    layout.addRow("Тип:", &typeEdit);
    layout.addRow("Цена за ночь:", &priceEdit);

    QPushButton saveBtn("Сохранить", &dlg);
    layout.addWidget(&saveBtn);

    connect(&saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted) {
        if (numEdit.text().isEmpty() || typeEdit.text().isEmpty() || priceEdit.text().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Заполните все поля!");
            return;
        }

        Room room;
        room.id = roomId;
        room.roomNumber = numEdit.text().toInt();
        room.type = typeEdit.text().trimmed();
        room.pricePerNight = priceEdit.text().toDouble();

        if (DBManager::instance().updateRoom(room)) {
            loadRooms();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось обновить номер.");
        }
    }
}

void MainWindow::deleteSelectedRoom() {
    int row = m_roomsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите номер для удаления!");
        return;
    }

    int roomId = m_roomsTable->item(row, 0)->text().toInt();
    auto reply = QMessageBox::question(
        this,
        "Подтверждение",
        "Вы действительно хотите удалить выбранный номер?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QString errorMsg;
        if (DBManager::instance().deleteRoom(roomId, errorMsg)) {
            loadRooms();
        } else {
            QMessageBox::critical(this, "Ошибка", errorMsg);
        }
    }
}

void MainWindow::addServiceDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle("Добавить услугу");
    QFormLayout layout(&dlg);

    QLineEdit nameEdit;
    QLineEdit priceEdit;
    priceEdit.setValidator(new QDoubleValidator(0.0, 100000.0, 2, &dlg));

    layout.addRow("Название:", &nameEdit);
    layout.addRow("Цена:", &priceEdit);

    QPushButton saveBtn("Сохранить", &dlg);
    layout.addWidget(&saveBtn);

    connect(&saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted) {
        if (nameEdit.text().isEmpty() || priceEdit.text().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Заполните все поля!");
            return;
        }

        Service service;
        service.name = nameEdit.text().trimmed();
        service.price = priceEdit.text().toDouble();

        if (DBManager::instance().addService(service)) {
            loadServices();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось добавить услугу.");
        }
    }
}

void MainWindow::editServiceDialog() {
    int row = m_servicesTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите услугу для редактирования!");
        return;
    }

    int serviceId = m_servicesTable->item(row, 0)->text().toInt();

    QDialog dlg(this);
    dlg.setWindowTitle("Изменить услугу");
    QFormLayout layout(&dlg);

    QLineEdit nameEdit(m_servicesTable->item(row, 1)->text());
    QLineEdit priceEdit(m_servicesTable->item(row, 2)->text());
    priceEdit.setValidator(new QDoubleValidator(0.0, 100000.0, 2, &dlg));

    layout.addRow("Название:", &nameEdit);
    layout.addRow("Цена:", &priceEdit);

    QPushButton saveBtn("Сохранить", &dlg);
    layout.addWidget(&saveBtn);

    connect(&saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() == QDialog::Accepted) {
        if (nameEdit.text().isEmpty() || priceEdit.text().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Заполните все поля!");
            return;
        }

        Service service;
        service.id = serviceId;
        service.name = nameEdit.text().trimmed();
        service.price = priceEdit.text().toDouble();

        if (DBManager::instance().updateService(service)) {
            loadServices();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось обновить услугу.");
        }
    }
}

void MainWindow::deleteSelectedService() {
    int row = m_servicesTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите услугу для удаления!");
        return;
    }

    int serviceId = m_servicesTable->item(row, 0)->text().toInt();
    auto reply = QMessageBox::question(
        this,
        "Подтверждение",
        "Вы действительно хотите удалить выбранную услугу?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QString errorMsg;
        if (DBManager::instance().deleteService(serviceId, errorMsg)) {
            loadServices();
        } else {
            QMessageBox::critical(this, "Ошибка", errorMsg);
        }
    }
}

void MainWindow::showGuestHistory() {
    int row = m_guestsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите гостя для просмотра истории!");
        return;
    }

    int guestId = m_guestsTable->item(row, 0)->text().toInt();
    QString guestName = m_guestsTable->item(row, 1)->text();

    QDialog dlg(this);
    dlg.setWindowTitle("История гостя: " + guestName);
    dlg.resize(650, 400);

    auto *layout = new QVBoxLayout(&dlg);
    auto *table = new QTableWidget(&dlg);
    configureTable(table, {"ID", "Номер", "Заезд", "Выезд", "Стоимость", "Статус"});

    for (const auto &booking : DBManager::instance().getGuestHistory(guestId)) {
        addTableRow(table, {
            QString::number(booking.id),
            QString::number(booking.roomNumber),
            booking.checkIn.toString("dd.MM.yyyy"),
            booking.checkOut.toString("dd.MM.yyyy"),
            QString::number(booking.totalCost, 'f', 2),
            booking.status
        });
    }

    layout->addWidget(table);
    dlg.exec();
}