#include "starmapview.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include <QPen>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QtMath>

namespace {
constexpr int StarItemType = QGraphicsItem::UserType + 1;

class StarItem final : public QGraphicsEllipseItem {
public:
    StarItem(const QString &entryId, const QRectF &rect) : QGraphicsEllipseItem(rect), m_entryId(entryId) {
        setAcceptHoverEvents(true);
        setAcceptedMouseButtons(Qt::LeftButton);
    }

    int type() const override { return StarItemType; }
    QString entryId() const { return m_entryId; }

private:
    QString m_entryId;
};
}

StarMapView::StarMapView(QWidget *parent) : QGraphicsView(parent) {
    auto *starScene = new QGraphicsScene(this);
    setScene(starScene);
    setRenderHint(QPainter::Antialiasing, true);
    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(QColor("#081120"));
    setSceneRect(QRectF(0, 0, 900, 620));
}

void StarMapView::setEntries(const QList<EmotionEntry> &entries) {
    m_entries = entries;
    rebuildScene();
}

void StarMapView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    fitInView(sceneRect(), Qt::KeepAspectRatioByExpanding);
}

void StarMapView::mousePressEvent(QMouseEvent *event) {
    if (auto *graphicsItem = itemAt(event->pos())) {
        if (graphicsItem->type() == StarItemType) {
            auto *starItem = static_cast<StarItem *>(graphicsItem);
            emit entrySelected(starItem->entryId());
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void StarMapView::rebuildScene() {
    scene()->clear();
    scene()->setSceneRect(QRectF(0, 0, 900, 620));

    for (int i = 0; i < 120; ++i) {
        const qreal x = QRandomGenerator::global()->bounded(900.0);
        const qreal y = QRandomGenerator::global()->bounded(620.0);
        const qreal size = 1.0 + QRandomGenerator::global()->generateDouble() * 2.2;
        auto *bgStar = scene()->addEllipse(x, y, size, size, Qt::NoPen, QColor(255, 255, 255, 120));
        bgStar->setZValue(-10);
    }

    for (int index = 0; index < m_entries.size(); ++index) {
        const auto &entry = m_entries.at(index);
        const QPointF center = positionForEntry(index, entry);
        const qreal radius = 12.0 + entry.intensity * 4.0;
        auto *item = new StarItem(entry.id, QRectF(center.x() - radius, center.y() - radius, radius * 2, radius * 2));
        item->setBrush(EmotionEntry::colorForEmotion(entry.emotion));
        item->setPen(QPen(Qt::white, 1.5));
        item->setToolTip(QString("%1\n%2").arg(entry.title, entry.emotion));
        scene()->addItem(item);

        auto *label = scene()->addText(entry.title.left(10));
        label->setDefaultTextColor(Qt::white);
        label->setPos(center.x() + radius + 6, center.y() - radius);
    }
}

QPointF StarMapView::positionForEntry(int index, const EmotionEntry &entry) const {
    const qreal baseX = 120.0 + (entry.date.dayOfYear() % 320) * 2.0;
    const qreal baseY = 120.0 + (entry.energy * 75.0) + (index % 3) * 28.0;
    const qreal wave = 55.0 * qSin((index + 1) * 0.85);
    return QPointF(qBound(60.0, baseX + wave, 840.0), qBound(60.0, baseY, 560.0));
}
