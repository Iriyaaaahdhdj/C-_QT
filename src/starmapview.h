#pragma once

#include <QList>
#include <QPointF>
#include <QVector>
#include <QTimer>
#include <QVector3D>
#include <QWidget>

#include "emotionentry.h"

class StarMapView : public QWidget {
    Q_OBJECT

public:
    explicit StarMapView(QWidget *parent = nullptr);

    void setEntries(const QList<EmotionEntry> &entries);
    void setSelectedEntryId(const QString &id);

signals:
    void entrySelected(const QString &id);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct BackgroundStar {
        QVector3D position;
        qreal size = 1.0;
        qreal alpha = 0.5;
    };

    struct ScreenStar {
        QString id;
        QString title;
        QString emotion;
        QString bagua;
        QString ganzhi;
        QPointF screenPos;
        QVector3D worldPos;
        qreal radius = 0.0;
        qreal depth = 0.0;
        QColor color;
    };

    void initializeBackground();
    QVector3D rotatePoint(const QVector3D &point) const;
    QVector3D entryWorldPosition(int index, const EmotionEntry &entry) const;
    bool projectPoint(const QVector3D &point, QPointF &screen, qreal &scale, qreal &depth) const;
    void rebuildProjectedStars();
    QString pickStar(const QPointF &pos) const;
    void tickCamera();

    QList<EmotionEntry> m_entries;
    QList<BackgroundStar> m_backgroundStars;
    QList<ScreenStar> m_projectedStars;
    QString m_selectedEntryId;
    QString m_hoveredEntryId;
    qreal m_yaw = 0.0;
    qreal m_pitch = 0.0;
    qreal m_yawVelocity = 0.0;
    qreal m_pitchVelocity = 0.0;
    bool m_dragging = false;
    bool m_dragMoved = false;
    QPointF m_lastMousePos;
    QPointF m_pressPos;
    QTimer m_animationTimer;
};
