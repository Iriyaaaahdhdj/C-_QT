#pragma once

#include <QList>
#include <QString>

#include "emotionentry.h"

class JournalRepository {
public:
    explicit JournalRepository(QString filePath);

    QList<EmotionEntry> load() const;
    bool save(const QList<EmotionEntry> &entries) const;

private:
    QString m_filePath;
};
