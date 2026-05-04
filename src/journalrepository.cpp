#include "journalrepository.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

JournalRepository::JournalRepository(QString filePath) : m_filePath(std::move(filePath)) {}

QList<EmotionEntry> JournalRepository::load() const {
    QFile file(m_filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QList<EmotionEntry> entries;
    for (const auto &value : doc.array()) {
        if (value.isObject()) {
            entries.append(EmotionEntry::fromJson(value.toObject()));
        }
    }
    return entries;
}

bool JournalRepository::save(const QList<EmotionEntry> &entries) const {
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QJsonArray array;
    for (const auto &entry : entries) {
        array.append(entry.toJson());
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
