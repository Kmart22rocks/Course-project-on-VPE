#include "mainwindow.h"
#include "bookingdialog.h"
#include "guesthistorydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Система управления гостиничным комплексом");
    resize(1020, 640);

    if (!m_db.openDatabase()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть базу данных!");
    }

    m_tcpServer = new HotelTcpServer(&m_db, this);
    m_tcpServer->startServer(0);

    m_tcpClient = new HotelTcpClient(this);
    m_apiClient = new CurrencyApiClient(this);

    setupUI();

    connect(m_tcpClient, &HotelTcpClient::summaryReceived,
            m_analyticsWidget, &AnalyticsWidget::updateAnalyticsData);
    connect(m_apiClient, &CurrencyApiClient::ratesUpdated,
            this, [this](double usd, double eur, bool fromNet) {
        m_analyticsWidget->updateCurrencyDisplay(usd, eur, fromNet);
        statusBar()->showMessage(
            QString("TCP-сервер активен (порт %1) | Курс ЦБ РФ: USD = %2 руб., EUR = %3 руб.")
                .arg(m_tcpServer->serverPort())
                .arg(usd, 0, 'f', 2)
                .arg(eur, 0, 'f', 2)
        );
    });

    loadRooms();
    loadServices();
    loadGuests();
    loadBookings();
    refreshNetworkAnalytics();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI() {
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    // Вкладка 1: Номера
    auto *roomsWidget = new QWidget();
    auto *roomsLayout = new QVBoxLayout(roomsWidget);
    m_roomsTable = new QTableWidget();
    m_roomsTable->setColumnCount(5);
    m_roomsTable->setHorizontalHeaderLabels({"ID", "Номер комнаты", "Категория", "Цена за ночь", "Статус"});
    m_roomsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_roomsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_roomsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    roomsLayout->addWidget(m_roomsTable);

    auto *roomsBtnLayout = new QHBoxLayout();
    auto *addRoomBtn = new QPushButton("Добавить номер");
    auto *editRoomBtn = new QPushButton("Редактировать номер");
    auto *delRoomBtn = new QPushButton("Удалить номер");
    roomsBtnLayout->addWidget(addRoomBtn);
    roomsBtnLayout->addWidget(editRoomBtn);
    roomsBtnLayout->addWidget(delRoomBtn);
    roomsLayout->addLayout(roomsBtnLayout);
    m_tabs->addTab(roomsWidget, "Номера");

    // Вкладка 2: Бронирования
    auto *bookingsWidget = new QWidget();
    auto *bookingsLayout = new QVBoxLayout(bookingsWidget);
    m_bookingsTable = new QTableWidget();
    m_bookingsTable->setColumnCount(8);
    m_bookingsTable->setHorizontalHeaderLabels({"ID", "Гость", "Комната", "Заезд", "Выезд", "Стоимость", "Статус в отеле", "Статус"});
    m_bookingsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_bookingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bookingsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bookingsLayout->addWidget(m_bookingsTable);

    auto *bookingsBtnLayout = new QHBoxLayout();
    auto *newBookingBtn = new QPushButton("Новое бронирование");
    auto *editBookingBtn = new QPushButton("Редактировать бронь");
    auto *cancelBookingBtn = new QPushButton("Отменить бронь");
    bookingsBtnLayout->addWidget(newBookingBtn);
    bookingsBtnLayout->addWidget(editBookingBtn);
    bookingsBtnLayout->addWidget(cancelBookingBtn);
    bookingsLayout->addLayout(bookingsBtnLayout);
    m_tabs->addTab(bookingsWidget, "Бронирования");

    // Вкладка 3: Гости и История
    auto *guestsWidget = new QWidget();
    auto *guestsLayout = new QVBoxLayout(guestsWidget);
    m_guestsTable = new QTableWidget();
    m_guestsTable->setColumnCount(4);
    m_guestsTable->setHorizontalHeaderLabels({"ID", "ФИО Гостя", "Паспорт", "Телефон"});
    m_guestsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_guestsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_guestsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    guestsLayout->addWidget(m_guestsTable);

    auto *guestsBtnLayout = new QHBoxLayout();
    auto *historyBtn = new QPushButton("История гостя");
    guestsBtnLayout->addWidget(historyBtn);
    guestsBtnLayout->addStretch();
    guestsLayout->addLayout(guestsBtnLayout);
    m_tabs->addTab(guestsWidget, "Гости и История");

    // Вкладка 4: Услуги
    auto *servicesWidget = new QWidget();
    auto *servicesLayout = new QVBoxLayout(servicesWidget);
    m_servicesTable = new QTableWidget();
    m_servicesTable->setColumnCount(3);
    m_servicesTable->setHorizontalHeaderLabels({"ID", "Наименование услуги", "Стоимость"});
    m_servicesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_servicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_servicesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    servicesLayout->addWidget(m_servicesTable);

    auto *servicesBtnLayout = new QHBoxLayout();
    auto *addServiceBtn = new QPushButton("Добавить услугу");
    auto *editServiceBtn = new QPushButton("Редактировать услугу");
    auto *delServiceBtn = new QPushButton("Удалить услугу");
    servicesBtnLayout->addWidget(addServiceBtn);
    servicesBtnLayout->addWidget(editServiceBtn);
    servicesBtnLayout->addWidget(delServiceBtn);
    servicesLayout->addLayout(servicesBtnLayout);
    m_tabs->addTab(servicesWidget, "Услуги");

    // Вкладка 5: Визуализация и Сеть (TCP + HTTP API)
    m_analyticsWidget = new AnalyticsWidget(this);
    m_tabs->addTab(m_analyticsWidget, "Аналитика и Сеть");

    // Подключение сигналов и слотов
    connect(addRoomBtn, &QPushButton::clicked, this, &MainWindow::addRoom);
    connect(editRoomBtn, &QPushButton::clicked, this, &MainWindow::editRoom);
    connect(delRoomBtn, &QPushButton::clicked, this, &MainWindow::deleteRoom);

    connect(addServiceBtn, &QPushButton::clicked, this, &MainWindow::addService);
    connect(editServiceBtn, &QPushButton::clicked, this, &MainWindow::editService);
    connect(delServiceBtn, &QPushButton::clicked, this, &MainWindow::deleteService);

    connect(historyBtn, &QPushButton::clicked, this, &MainWindow::viewGuestHistory);

    connect(newBookingBtn, &QPushButton::clicked, this, &MainWindow::createBooking);
    connect(editBookingBtn, &QPushButton::clicked, this, &MainWindow::editBooking);
    connect(cancelBookingBtn, &QPushButton::clicked, this, &MainWindow::cancelBooking);

    connect(m_analyticsWidget, &AnalyticsWidget::refreshRequested,
            this, &MainWindow::refreshNetworkAnalytics);

    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int idx){
        if (idx == 0) loadRooms();
        else if (idx == 1) loadBookings();
        else if (idx == 2) loadGuests();
        else if (idx == 3) loadServices();
        else if (idx == 4) refreshNetworkAnalytics();
    });
}

void MainWindow::refreshNetworkAnalytics() {
    m_tcpClient->requestHotelSummary("127.0.0.1", m_tcpServer->serverPort());
    m_apiClient->fetchRates();
}

void MainWindow::loadRooms() {
    QList<Room> rooms = m_db.getAllRooms();
    m_roomsTable->setRowCount(rooms.size());
    for (int i = 0; i < rooms.size(); ++i) {
        m_roomsTable->setItem(i, 0, new QTableWidgetItem(QString::number(rooms[i].id)));
        m_roomsTable->setItem(i, 1, new QTableWidgetItem(QString::number(rooms[i].roomNumber)));
        m_roomsTable->setItem(i, 2, new QTableWidgetItem(rooms[i].type));
        m_roomsTable->setItem(i, 3, new QTableWidgetItem(QString("%1 руб.").arg(rooms[i].pricePerNight, 0, 'f', 2)));
        m_roomsTable->setItem(i, 4, new QTableWidgetItem(rooms[i].status));
    }
}

void MainWindow::addRoom() {
    QDialog dlg(this);
    dlg.setWindowTitle("Добавление номера");
    auto *layout = new QFormLayout(&dlg);

    auto *numSpin = new QSpinBox(&dlg);
    numSpin->setRange(1, 9999);
    numSpin->setValue(103);

    auto *typeCombo = new QComboBox(&dlg);
    typeCombo->addItems({"Стандарт", "Комфорт", "Люкс", "Президентский"});

    auto *priceSpin = new QDoubleSpinBox(&dlg);
    priceSpin->setRange(100.0, 100000.0);
    priceSpin->setValue(3000.0);

    layout->addRow("Номер комнаты:", numSpin);
    layout->addRow("Категория:", typeCombo);
    layout->addRow("Цена за сутки:", priceSpin);

    auto *saveBtn = new QPushButton("Добавить", &dlg);
    connect(saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addRow(saveBtn);

    if (dlg.exec() == QDialog::Accepted) {
        Room r;
        r.roomNumber = numSpin->value();
        r.type = typeCombo->currentText();
        r.pricePerNight = priceSpin->value();
        r.status = "Свободен";
        if (m_db.addRoom(r)) {
            loadRooms();
            refreshNetworkAnalytics();
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось добавить номер. Возможно, такой номер уже существует!");
        }
    }
}

void MainWindow::editRoom() {
    int row = m_roomsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Инфо", "Выберите номер для редактирования!");
        return;
    }

    int id = m_roomsTable->item(row, 0)->text().toInt();
    int currentNum = m_roomsTable->item(row, 1)->text().toInt();
    QString currentType = m_roomsTable->item(row, 2)->text();
    QString priceStr = m_roomsTable->item(row, 3)->text();
    priceStr.remove(" руб.");
    double currentPrice = priceStr.toDouble();
    QString currentStatus = m_roomsTable->item(row, 4)->text();

    QDialog dlg(this);
    dlg.setWindowTitle("Редактирование номера");
    auto *layout = new QFormLayout(&dlg);

    auto *numSpin = new QSpinBox(&dlg);
    numSpin->setRange(1, 9999);
    numSpin->setValue(currentNum);

    auto *typeCombo = new QComboBox(&dlg);
    typeCombo->addItems({"Стандарт", "Комфорт", "Люкс", "Президентский"});
    typeCombo->setCurrentText(currentType);

    auto *priceSpin = new QDoubleSpinBox(&dlg);
    priceSpin->setRange(100.0, 100000.0);
    priceSpin->setValue(currentPrice);

    auto *statusCombo = new QComboBox(&dlg);
    statusCombo->addItems({"Свободен", "Ремонт"});
    statusCombo->setCurrentText(currentStatus);

    layout->addRow("Номер комнаты:", numSpin);
    layout->addRow("Категория:", typeCombo);
    layout->addRow("Цена за сутки:", priceSpin);
    layout->addRow("Статус:", statusCombo);

    auto *saveBtn = new QPushButton("Сохранить", &dlg);
    connect(saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addRow(saveBtn);

    if (dlg.exec() == QDialog::Accepted) {
        Room r;
        r.id = id;
        r.roomNumber = numSpin->value();
        r.type = typeCombo->currentText();
        r.pricePerNight = priceSpin->value();
        r.status = statusCombo->currentText();

        if (m_db.updateRoom(r)) {
            loadRooms();
            refreshNetworkAnalytics();
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось обновить параметры номера!");
        }
    }
}

void MainWindow::deleteRoom() {
    int row = m_roomsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Инфо", "Выберите номер для удаления!");
        return;
    }

    int id = m_roomsTable->item(row, 0)->text().toInt();
    int num = m_roomsTable->item(row, 1)->text().toInt();

    auto reply = QMessageBox::question(
        this, "Подтверждение",
        QString("Вы действительно хотите удалить номер №%1?").arg(num),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        QString errorMsg;
        if (m_db.deleteRoom(id, errorMsg)) {
            loadRooms();
            refreshNetworkAnalytics();
        } else {
            QMessageBox::critical(this, "Ошибка удаления", errorMsg);
        }
    }
}

void MainWindow::loadServices() {
    QList<Service> list = m_db.getAllServices();
    m_servicesTable->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        m_servicesTable->setItem(i, 0, new QTableWidgetItem(QString::number(list[i].id)));
        m_servicesTable->setItem(i, 1, new QTableWidgetItem(list[i].name));
        m_servicesTable->setItem(i, 2, new QTableWidgetItem(QString("%1 руб.").arg(list[i].price, 0, 'f', 2)));
    }
}

void MainWindow::addService() {
    bool ok;
    QString name = QInputDialog::getText(this, "Новая услуга", "Наименование услуги:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    double price = QInputDialog::getDouble(this, "Новая услуга", "Стоимость (руб.):", 500.0, 0.0, 50000.0, 2, &ok);
    if (!ok) return;

    Service s{0, name.trimmed(), price};
    if (m_db.addService(s)) {
        loadServices();
    }
}

void MainWindow::editService() {
    int row = m_servicesTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Инфо", "Выберите услугу для редактирования!");
        return;
    }

    int id = m_servicesTable->item(row, 0)->text().toInt();
    QString currentName = m_servicesTable->item(row, 1)->text();
    QString priceStr = m_servicesTable->item(row, 2)->text();
    priceStr.remove(" руб.");
    double currentPrice = priceStr.toDouble();

    bool ok;
    QString name = QInputDialog::getText(this, "Редактирование", "Наименование услуги:", QLineEdit::Normal, currentName, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    double price = QInputDialog::getDouble(this, "Редактирование", "Стоимость (руб.):", currentPrice, 0.0, 50000.0, 2, &ok);
    if (!ok) return;

    Service s{id, name.trimmed(), price};
    if (m_db.updateService(s)) {
        loadServices();
    }
}

void MainWindow::deleteService() {
    int row = m_servicesTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Инфо", "Выберите услугу для удаления!");
        return;
    }

    int id = m_servicesTable->item(row, 0)->text().toInt();
    auto reply = QMessageBox::question(this, "Подтверждение", "Удалить выбранную услугу?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QString err;
        if (m_db.deleteService(id, err)) {
            loadServices();
        } else {
            QMessageBox::warning(this, "Ошибка", err);
        }
    }
}

void MainWindow::loadGuests() {
    QList<Guest> list = m_db.getAllGuests();
    m_guestsTable->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        m_guestsTable->setItem(i, 0, new QTableWidgetItem(QString::number(list[i].id)));
        m_guestsTable->setItem(i, 1, new QTableWidgetItem(list[i].fullName));
        m_guestsTable->setItem(i, 2, new QTableWidgetItem(list[i].passport));
        m_guestsTable->setItem(i, 3, new QTableWidgetItem(list[i].phone));
    }
}

void MainWindow::viewGuestHistory() {
    int row = m_guestsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Инфо", "Выберите гостя для просмотра истории!");
        return;
    }

    Guest g;
    g.id = m_guestsTable->item(row, 0)->text().toInt();
    g.fullName = m_guestsTable->item(row, 1)->text();
    g.passport = m_guestsTable->item(row, 2)->text();
    g.phone = m_guestsTable->item(row, 3)->text();

    GuestHistoryDialog dlg(g, &m_db, this);
    dlg.exec();
}

void MainWindow::loadBookings() {
    QList<Booking> list = m_db.getAllBookings();
    m_bookingsTable->setRowCount(list.size());
    QDate today = QDate::currentDate();

    for (int i = 0; i < list.size(); ++i) {
        const auto &b = list[i];
        m_bookingsTable->setItem(i, 0, new QTableWidgetItem(QString::number(b.id)));
        m_bookingsTable->setItem(i, 1, new QTableWidgetItem(b.guestName));
        m_bookingsTable->setItem(i, 2, new QTableWidgetItem(QString("№%1").arg(b.roomNumber)));
        m_bookingsTable->setItem(i, 3, new QTableWidgetItem(b.checkIn.toString("dd.MM.yyyy")));
        m_bookingsTable->setItem(i, 4, new QTableWidgetItem(b.checkOut.toString("dd.MM.yyyy")));
        m_bookingsTable->setItem(i, 5, new QTableWidgetItem(QString("%1 руб.").arg(b.totalCost, 0, 'f', 2)));

        QString hotelState = (b.checkIn <= today) ? "Проживает" : "Будущее бронирование";
        m_bookingsTable->setItem(i, 6, new QTableWidgetItem(hotelState));
        m_bookingsTable->setItem(i, 7, new QTableWidgetItem(b.status));
    }
}

void MainWindow::createBooking() {
    BookingDialog dlg(&m_db, m_apiClient, this);
    if (dlg.exec() == QDialog::Accepted) {
        Booking b = dlg.getBookingData();
        if (m_db.addBooking(b)) {
            loadBookings();
            loadGuests();
            loadRooms();
            refreshNetworkAnalytics();
            QMessageBox::information(this, "Успех", "Бронирование успешно оформлено!");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось сохранить бронирование!");
        }
    }
}

void MainWindow::editBooking() {
    int row = m_bookingsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Инфо", "Выберите бронирование для редактирования!");
        return;
    }

    int bookingId = m_bookingsTable->item(row, 0)->text().toInt();
    BookingDialog dlg(&m_db, m_apiClient, this, bookingId);
    if (dlg.exec() == QDialog::Accepted) {
        Booking b = dlg.getBookingData();
        b.id = bookingId;
        if (m_db.updateBooking(b)) {
            loadBookings();
            loadGuests();
            loadRooms();
            refreshNetworkAnalytics();
            QMessageBox::information(this, "Успех", "Данные бронирования успешно обновлены!");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось обновить бронирование!");
        }
    }
}

void MainWindow::cancelBooking() {
    int row = m_bookingsTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Инфо", "Выберите бронирование для отмены!");
        return;
    }

    int bookingId = m_bookingsTable->item(row, 0)->text().toInt();
    auto reply = QMessageBox::question(this, "Подтверждение", "Вы уверены, что хотите отменить эту бронь?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        if (m_db.cancelBooking(bookingId)) {
            loadBookings();
            loadRooms();
            refreshNetworkAnalytics();
            QMessageBox::information(this, "Успех", "Бронирование отменено!");
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось отменить бронирование!");
        }
    }
}