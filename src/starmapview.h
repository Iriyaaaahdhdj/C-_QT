#pragma once

#include <QGraphicsView>
#include <QList>

#include "emotionentry.h"

class StarMapView : public QGraphicsView {
    Q_OBJECT

public:
    explicit StarMapView(QWidget *parent = nullptr);

    void setEntries(const QList<EmotionEntry> &entries);

signals:
    void entrySelected(const QString &id);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildScene();
    QPointF positionForEntry(int index, const EmotionEntry &entry) const;

    QList<EmotionEntry> m_entries;
};
