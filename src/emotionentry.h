#pragma once

#include <QColor>
#include <QDate>
#include <QJsonObject>
#include <QString>

struct EmotionEntry {
    QString id;
    QString title;
    QString emotion;
    QString note;
    QDate date;
    int intensity = 3;
    int energy = 3;

    static QColor colorForEmotion(const QString &emotion) {
        if (emotion == "Joy") return QColor("#f6c445");
        if (emotion == "Calm") return QColor("#5cc8ff");
        if (emotion == "Sadness") return QColor("#7a7cff");
        if (emotion == "Anger") return QColor("#ff6b57");
        if (emotion == "Anxiety") return QColor("#b980ff");
        return QColor("#9ad18b");
    }

    static QString iconForEmotion(const QString &emotion) {
        if (emotion == "Joy") return "Sun";
        if (emotion == "Calm") return "Moon";
        if (emotion == "Sadness") return "Rain";
        if (emotion == "Anger") return "Spark";
        if (emotion == "Anxiety") return "Fog";
        return "Leaf";
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["title"] = title;
        obj["emotion"] = emotion;
        obj["note"] = note;
        obj["date"] = date.toString(Qt::ISODate);
        obj["intensity"] = intensity;
        obj["energy"] = energy;
        return obj;
    }

    static EmotionEntry fromJson(const QJsonObject &obj) {
        EmotionEntry entry;
        entry.id = obj["id"].toString();
        entry.title = obj["title"].toString();
        entry.emotion = obj["emotion"].toString();
        entry.note = obj["note"].toString();
        entry.date = QDate::fromString(obj["date"].toString(), Qt::ISODate);
        entry.intensity = obj["intensity"].toInt(3);
        entry.energy = obj["energy"].toInt(3);
        return entry;
    }
};
