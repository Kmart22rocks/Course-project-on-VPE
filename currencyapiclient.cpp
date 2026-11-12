#include "currencyapiclient.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>

CurrencyApiClient::CurrencyApiClient(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &CurrencyApiClient::onReplyFinished);
}

void CurrencyApiClient::fetchRates(const QUrl &url) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "HotelManagementSystem/1.0");
    m_networkManager->get(request);
}

bool CurrencyApiClient::parseRatesJson(const QByteArray &jsonData) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("Valute") || !root["Valute"].isObject()) {
        return false;
    }

    QJsonObject valute = root["Valute"].toObject();
    double usd = valute["USD"].toObject()["Value"].toDouble();
    double eur = valute["EUR"].toObject()["Value"].toDouble();

    if (usd <= 0.0 || eur <= 0.0) {
        return false;
    }

    m_usdRate = usd;
    m_eurRate = eur;
    return true;
}

void CurrencyApiClient::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray payload = reply->readAll();
        if (parseRatesJson(payload)) {
            emit ratesUpdated(m_usdRate, m_eurRate, true);
            reply->deleteLater();
            return;
        }
    }

    emit apiErrorOccurred(reply->errorString());
    emit ratesUpdated(m_usdRate, m_eurRate, false);
    reply->deleteLater();
}