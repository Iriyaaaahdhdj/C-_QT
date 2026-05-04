#pragma once

#include <QColor>
#include <QDate>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

struct EmotionEntry {
    QString id;
    QString title;
    QString emotion;
    QString note;
    QStringList tags;
    QDate date;
    int intensity = 3;
    int energy = 3;

    static QString normalizedEmotion(const QString &emotion) {
        if (emotion == "Joy" || emotion == "快乐") return "快乐";
        if (emotion == "Calm" || emotion == "平静") return "平静";
        if (emotion == "Sadness" || emotion == "难过") return "难过";
        if (emotion == "Anger" || emotion == "愤怒") return "愤怒";
        if (emotion == "Anxiety" || emotion == "焦虑") return "焦虑";
        if (emotion == "Hope" || emotion == "希望") return "希望";
        return emotion;
    }

    static QColor colorForEmotion(const QString &emotion) {
        const auto value = normalizedEmotion(emotion);
        if (value == "快乐") return QColor("#f2d7a8");
        if (value == "平静") return QColor("#c8ddff");
        if (value == "难过") return QColor("#cdd2f6");
        if (value == "愤怒") return QColor("#f2c7bb");
        if (value == "焦虑") return QColor("#d7cef5");
        return QColor("#d4ebcb");
    }

    static QString iconForEmotion(const QString &emotion) {
        const auto value = normalizedEmotion(emotion);
        if (value == "快乐") return "晴";
        if (value == "平静") return "月";
        if (value == "难过") return "雨";
        if (value == "愤怒") return "焰";
        if (value == "焦虑") return "雾";
        return "芽";
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

        QJsonArray tagArray;
        for (const auto &tag : tags) {
            tagArray.append(tag);
        }
        obj["tags"] = tagArray;
        return obj;
    }

    static EmotionEntry fromJson(const QJsonObject &obj) {
        EmotionEntry entry;
        entry.id = obj["id"].toString();
        entry.title = obj["title"].toString();
        entry.emotion = normalizedEmotion(obj["emotion"].toString());
        entry.note = obj["note"].toString();
        entry.date = QDate::fromString(obj["date"].toString(), Qt::ISODate);
        entry.intensity = obj["intensity"].toInt(3);
        entry.energy = obj["energy"].toInt(3);

        if (obj.contains("tags") && obj["tags"].isArray()) {
            const QJsonArray tagArray = obj["tags"].toArray();
            for (const auto &value : tagArray) {
                if (value.isString()) {
                    entry.tags.append(value.toString());
                }
            }
        }
        return entry;
    }
};
