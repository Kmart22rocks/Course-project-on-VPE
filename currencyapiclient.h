#ifndef CURRENCYAPICLIENT_H
#define CURRENCYAPICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

class CurrencyApiClient : public QObject {
    Q_OBJECT
public:
    explicit CurrencyApiClient(QObject *parent = nullptr);

    void fetchRates(const QUrl &url = QUrl("https://www.cbr-xml-daily.ru/daily_json.js"));
    bool parseRatesJson(const QByteArray &jsonData);

    double usdRate() const { return m_usdRate; }
    double eurRate() const { return m_eurRate; }

    signals:
        void ratesUpdated(double usdRate, double eurRate, bool fromNetwork);
    void apiErrorOccurred(const QString &errorString);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    double m_usdRate = 90.0;
    double m_eurRate = 98.0;
};

#endif // CURRENCYAPICLIENT_H