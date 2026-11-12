#include "networkservice.h"
#include "bookingcalculator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>

// ==================== HotelTcpServer ====================

HotelTcpServer::HotelTcpServer(DBManager *db, QObject *parent)
    : QObject(parent), m_db(db), m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &HotelTcpServer::onNewConnection);
}

bool HotelTcpServer::startServer(quint16 port) {
    if (m_server->isListening()) {
        return true;
    }
    return m_server->listen(QHostAddress::LocalHost, port);
}

quint16 HotelTcpServer::serverPort() const {
    return m_server->serverPort();
}

bool HotelTcpServer::isListening() const {
    return m_server->isListening();
}

void HotelTcpServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket *clientSocket = m_server->nextPendingConnection();
        connect(clientSocket, &QTcpSocket::readyRead, this, &HotelTcpServer::onClientReadyRead);
        connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
    }
}

void HotelTcpServer::onClientReadyRead() {
    auto *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    while (socket->canReadLine()) {
        QByteArray line = socket->readLine().trimmed();
        QJsonDocument reqDoc = QJsonDocument::fromJson(line);
        QString cmd = reqDoc.object().value("cmd").toString();

        if (cmd == "get_summary") {
            QByteArray response = buildSummaryResponse();
            socket->write(response + "\n");
            socket->flush();
            emit requestProcessed(cmd);
        } else {
            QJsonObject errObj{{"status", "error"}, {"message", "Unknown command"}};
            socket->write(QJsonDocument(errObj).toJson(QJsonDocument::Compact) + "\n");
            socket->flush();
        }
    }
}

QByteArray HotelTcpServer::buildSummaryResponse() const {
    QList<Room> rooms = m_db->getAllRooms();
    QList<Booking> activeBookings = m_db->getAllBookings();
    QList<Guest> guests = m_db->getAllGuests();

    QList<Booking> allHistory;
    for (const auto &g : guests) {
        allHistory.append(m_db->getGuestHistory(g.id));
    }

    QMap<QString, double> revenueMap = BookingCalculator::calculateRevenueByCategory(rooms, allHistory);
    QJsonObject revJson;
    for (auto it = revenueMap.constBegin(); it != revenueMap.constEnd(); ++it) {
        revJson.insert(it.key(), it.value());
    }

    QJsonObject resp;
    resp["status"] = "ok";
    resp["total_rooms"] = rooms.size();
    resp["active_bookings"] = activeBookings.size();
    resp["total_guests"] = guests.size();
    resp["category_revenue"] = revJson;

    return QJsonDocument(resp).toJson(QJsonDocument::Compact);
}

// ==================== HotelTcpClient ====================

HotelTcpClient::HotelTcpClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)) {
    connect(m_socket, &QTcpSocket::connected, this, &HotelTcpClient::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &HotelTcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &HotelTcpClient::onErrorOccurred);
}

void HotelTcpClient::requestHotelSummary(const QString &host, quint16 port) {
    m_buffer.clear();
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
    m_socket->connectToHost(host, port);
}

void HotelTcpClient::onConnected() {
    QJsonObject req{{"cmd", "get_summary"}};
    m_socket->write(QJsonDocument(req).toJson(QJsonDocument::Compact) + "\n");
    m_socket->flush();
}

void HotelTcpClient::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    int newlineIdx = m_buffer.indexOf('\n');
    if (newlineIdx == -1) return;

    QByteArray line = m_buffer.left(newlineIdx).trimmed();
    m_buffer.remove(0, newlineIdx + 1);

    QJsonDocument doc = QJsonDocument::fromJson(line);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    if (obj["status"].toString() == "ok") {
        int totalRooms = obj["total_rooms"].toInt();
        int activeBookings = obj["active_bookings"].toInt();
        int totalGuests = obj["total_guests"].toInt();

        QMap<QString, double> revenue;
        QJsonObject revObj = obj["category_revenue"].toObject();
        for (auto it = revObj.begin(); it != revObj.end(); ++it) {
            revenue.insert(it.key(), it.value().toDouble());
        }

        emit summaryReceived(totalRooms, activeBookings, totalGuests, revenue);
    }
    m_socket->disconnectFromHost();
}

void HotelTcpClient::onErrorOccurred(QAbstractSocket::SocketError) {
    emit networkError(m_socket->errorString());
}