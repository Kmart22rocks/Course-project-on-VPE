#ifndef NETWORKSERVICE_H
#define NETWORKSERVICE_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include "dbmanager.h"

class HotelTcpServer : public QObject {
    Q_OBJECT
public:
    explicit HotelTcpServer(DBManager *db, QObject *parent = nullptr);
    bool startServer(quint16 port = 45454);
    quint16 serverPort() const;
    bool isListening() const;

    signals:
        void requestProcessed(const QString &command);

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    QByteArray buildSummaryResponse() const;

    DBManager *m_db;
    QTcpServer *m_server;
};

class HotelTcpClient : public QObject {
    Q_OBJECT
public:
    explicit HotelTcpClient(QObject *parent = nullptr);
    void requestHotelSummary(const QString &host = "127.0.0.1", quint16 port = 45454);

    signals:
        void summaryReceived(int totalRooms,
                             int activeBookings,
                             int totalGuests,
                             const QMap<QString, double> &categoryRevenue);
    void networkError(const QString &message);

private slots:
    void onConnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer;
};

#endif // NETWORKSERVICE_H
