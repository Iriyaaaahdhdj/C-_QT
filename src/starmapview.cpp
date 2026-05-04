#include "starmapview.h"

#include <algorithm>

#include <QLineF>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QStringList>
#include <QtMath>

namespace {
constexpr int BackgroundStarCount = 1300;
constexpr int MilkyWayStarCount = 900;
constexpr qreal SphereRadiusFactor = 0.42;
constexpr qreal MaxPitch = 1.18;

const QStringList HeavenlyStems = {"甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬", "癸"};
const QStringList EarthlyBranches = {"子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌", "亥"};
const QStringList BaguaNames = {"乾", "兑", "离", "震", "巽", "坎", "艮", "坤"};

struct AstroMeta {
    QString ganzhi;
    QString bagua;
    qreal longitude = 0.0;
    qreal latitude = 0.0;
};

QVector3D sphericalToCartesian(qreal longitude, qreal latitude) {
    const qreal clat = qCos(latitude);
    return QVector3D(clat * qCos(longitude), qSin(latitude), clat * qSin(longitude));
}

int positiveMod(qint64 value, int mod) {
    const int result = int(value % mod);
    return result < 0 ? result + mod : result;
}

AstroMeta metaForEntry(const EmotionEntry &entry) {
    const qint64 daySeed = entry.date.toJulianDay();
    const uint contentHash = qHash(entry.title + "|" + entry.note + "|" + entry.emotion + "|" + entry.tags.join("|"));

    const int stemIndex = positiveMod(daySeed, HeavenlyStems.size());
    const int branchIndex = positiveMod(daySeed, EarthlyBranches.size());
    const int baguaIndex = positiveMod(contentHash, BaguaNames.size());

    AstroMeta meta;
    meta.ganzhi = HeavenlyStems.at(stemIndex) + EarthlyBranches.at(branchIndex);
    meta.bagua = BaguaNames.at(baguaIndex);

    meta.longitude =
        (-M_PI * 0.5) +
        baguaIndex * (M_PI / 4.0) +
        ((branchIndex % 3) - 1) * 0.1 +
        (entry.intensity - 3) * 0.03;

    const qreal emotionBias =
        entry.emotion == "快乐" ? 0.34 :
        entry.emotion == "平静" ? 0.08 :
        entry.emotion == "难过" ? -0.2 :
        entry.emotion == "愤怒" ? 0.22 :
        entry.emotion == "焦虑" ? -0.05 : 0.18;

    meta.latitude = qBound(-0.92,
                           emotionBias + (entry.energy - 3) * 0.11 + (branchIndex / 11.0 - 0.5) * 0.22,
                           0.92);
    return meta;
}

QColor starTemperatureColor(qreal tone) {
    if (tone < 0.2) return QColor("#dbe7ff");
    if (tone < 0.45) return QColor("#eef4ff");
    if (tone < 0.7) return QColor("#fff2de");
    return QColor("#ffd9c6");
}
}

StarMapView::StarMapView(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    initializeBackground();

    m_animationTimer.setInterval(16);
    connect(&m_animationTimer, &QTimer::timeout, this, &StarMapView::tickCamera);
    m_animationTimer.start();
}

void StarMapView::setEntries(const QList<EmotionEntry> &entries) {
    m_entries = entries;
    rebuildProjectedStars();
    update();
}

void StarMapView::setSelectedEntryId(const QString &id) {
    m_selectedEntryId = id;
    rebuildProjectedStars();
    update();
}

void StarMapView::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0.0, QColor("#01040b"));
    bg.setColorAt(0.32, QColor("#071224"));
    bg.setColorAt(0.74, QColor("#040b16"));
    bg.setColorAt(1.0, QColor("#010207"));
    painter.fillRect(rect(), bg);

    const QPointF center(width() * 0.54, height() * 0.5);
    const qreal radius = qMin(width(), height()) * SphereRadiusFactor;
    const QRectF sphereRect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

    QRadialGradient halo(center, radius * 1.28);
    halo.setColorAt(0.0, QColor(89, 124, 255, 24));
    halo.setColorAt(0.38, QColor(111, 93, 255, 16));
    halo.setColorAt(0.7, QColor(255, 161, 121, 8));
    halo.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(halo);
    painter.drawEllipse(center, radius * 1.28, radius * 1.28);

    QRadialGradient sphereFill(center.x() - radius * 0.24, center.y() - radius * 0.3, radius * 1.18);
    sphereFill.setColorAt(0.0, QColor("#193866"));
    sphereFill.setColorAt(0.25, QColor("#09172d"));
    sphereFill.setColorAt(1.0, QColor("#01040b"));
    painter.setBrush(sphereFill);
    painter.drawEllipse(sphereRect);

    painter.save();
    QPainterPath clip;
    clip.addEllipse(sphereRect);
    painter.setClipPath(clip);

    QPainterPath milkyBand;
    milkyBand.moveTo(center.x() - radius * 1.18, center.y() - radius * 0.18);
    milkyBand.cubicTo(center.x() - radius * 0.58, center.y() - radius * 0.56,
                      center.x() + radius * 0.16, center.y() + radius * 0.38,
                      center.x() + radius * 1.18, center.y() + radius * 0.06);
    painter.setPen(QPen(QColor(204, 219, 255, 16), radius * 0.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(milkyBand);

    QRadialGradient nebulaA(center.x() - radius * 0.38, center.y() - radius * 0.2, radius * 0.74);
    nebulaA.setColorAt(0.0, QColor(108, 154, 255, 26));
    nebulaA.setColorAt(0.6, QColor(72, 108, 220, 8));
    nebulaA.setColorAt(1.0, QColor(108, 154, 255, 0));
    painter.fillRect(sphereRect, nebulaA);

    QRadialGradient nebulaB(center.x() + radius * 0.24, center.y() + radius * 0.14, radius * 0.64);
    nebulaB.setColorAt(0.0, QColor(255, 135, 112, 18));
    nebulaB.setColorAt(0.7, QColor(255, 135, 112, 5));
    nebulaB.setColorAt(1.0, QColor(255, 135, 112, 0));
    painter.fillRect(sphereRect, nebulaB);

    painter.setPen(QPen(QColor(255, 255, 255, 10), 0.8));
    for (int i = -3; i <= 3; ++i) {
        const qreal lat = i * 0.28;
        QRectF latRect(center.x() - radius, center.y() - qCos(lat) * radius * 0.72, radius * 2, qCos(lat) * radius * 1.44);
        painter.drawEllipse(latRect);
    }
    for (int i = -4; i <= 4; ++i) {
        const qreal scale = 1.0 - qAbs(i) * 0.11;
        QRectF lonRect(center.x() - radius * scale, center.y() - radius, radius * 2 * scale, radius * 2);
        painter.drawEllipse(lonRect);
    }

    for (const auto &star : m_backgroundStars) {
        QPointF screen;
        qreal scale = 0.0;
        qreal depth = 0.0;
        if (!projectPoint(star.position, screen, scale, depth)) {
            continue;
        }

        QColor color = starTemperatureColor(qBound(0.0, scale, 1.0));
        color.setAlpha(qBound(12, int(star.alpha * 255 * (0.36 + depth * 0.74)), 180));
        const qreal size = qMax<qreal>(0.18, star.size * (0.16 + scale * 0.56));

        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(screen, size, size);
    }

    std::sort(m_projectedStars.begin(), m_projectedStars.end(), [](const ScreenStar &a, const ScreenStar &b) {
        return a.depth < b.depth;
    });

    if (m_projectedStars.size() > 1) {
        painter.setBrush(Qt::NoBrush);
        for (int i = 1; i < m_projectedStars.size(); ++i) {
            const auto &prev = m_projectedStars.at(i - 1);
            const auto &curr = m_projectedStars.at(i);

            QColor trailColor(189, 206, 244, 34);
            qreal penWidth = 0.75;
            if (curr.id == m_selectedEntryId || prev.id == m_selectedEntryId) {
                trailColor = QColor(255, 224, 176, 78);
                penWidth = 1.3;
            }

            painter.setPen(QPen(trailColor, penWidth, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(prev.screenPos, curr.screenPos);
        }
    }

    for (const auto &star : m_projectedStars) {
        const bool selected = star.id == m_selectedEntryId;
        const bool hovered = star.id == m_hoveredEntryId;
        const qreal glowRadius = star.radius * (selected ? 2.8 : hovered ? 2.35 : 1.85);

        QRadialGradient glow(star.screenPos, glowRadius);
        QColor glowColor = star.color;
        glowColor.setAlpha(selected ? 82 : hovered ? 56 : 34);
        glow.setColorAt(0.0, glowColor);
        glowColor.setAlpha(0);
        glow.setColorAt(1.0, glowColor);
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(star.screenPos, glowRadius, glowRadius);

        painter.setBrush(QColor(255, 250, 244, selected ? 246 : 230));
        painter.drawEllipse(star.screenPos, star.radius, star.radius);
        painter.setBrush(star.color);
        painter.drawEllipse(star.screenPos, star.radius * 0.28, star.radius * 0.28);

        painter.setPen(QPen(QColor(255, 249, 240, selected ? 120 : 72), 0.85));
        painter.drawLine(QPointF(star.screenPos.x() - star.radius * 0.72, star.screenPos.y()),
                         QPointF(star.screenPos.x() + star.radius * 0.72, star.screenPos.y()));
        painter.drawLine(QPointF(star.screenPos.x(), star.screenPos.y() - star.radius * 0.72),
                         QPointF(star.screenPos.x(), star.screenPos.y() + star.radius * 0.72));

        if (selected || hovered) {
            painter.setPen(QPen(QColor(255, 241, 208, selected ? 1.1 : 0.85), 0.9));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(star.screenPos, star.radius * 1.12, star.radius * 1.12);
        }
    }

    painter.restore();

    painter.setPen(QPen(QColor(255, 255, 255, 34), 1.1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(sphereRect);

    for (const auto &star : m_projectedStars) {
        if (star.id != m_selectedEntryId && star.id != m_hoveredEntryId) {
            continue;
        }

        const qreal bubbleWidth = qMin<qreal>(280.0, width() * 0.32);
        qreal labelX = star.screenPos.x() + 18;
        qreal labelY = star.screenPos.y() - 30;
        labelX = qBound<qreal>(16.0, labelX, width() - bubbleWidth - 16.0);
        labelY = qBound<qreal>(16.0, labelY, height() - 86.0);
        const QRectF labelRect(labelX, labelY, bubbleWidth, 70);

        QPainterPath bubble;
        bubble.addRoundedRect(labelRect, 16, 16);
        painter.fillPath(bubble, QColor(6, 12, 24, 190));

        painter.setPen(QColor("#f7fbff"));
        painter.drawText(labelRect.adjusted(14, 7, -14, -38), star.title);
        painter.setPen(QColor("#c6d6ef"));
        painter.drawText(labelRect.adjusted(14, 25, -14, -20), star.emotion + "  ·  " + star.ganzhi);
        painter.setPen(QColor("#8fa3c8"));
        painter.drawText(labelRect.adjusted(14, 44, -14, -6), "八卦方位：" + star.bagua);
    }

    const QRectF badgeRect(22, 20, qMin<qreal>(420.0, width() - 44.0), 34);
    QPainterPath badge;
    badge.addRoundedRect(badgeRect, 17, 17);
    painter.fillPath(badge, QColor(4, 10, 20, 150));
    painter.setPen(QColor("#dbe6fb"));
    painter.drawText(badgeRect.adjusted(14, 8, -14, -8), QStringLiteral("按住拖动旋转你的情绪星空，细线星轨会按记录日期依次相连"));
}

void StarMapView::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging) {
        const QPointF delta = event->position() - m_lastMousePos;
        m_dragMoved = m_dragMoved || delta.manhattanLength() > 1.5;
        m_yaw += delta.x() * 0.0082;
        m_pitch = qBound(-MaxPitch, m_pitch - delta.y() * 0.006, MaxPitch);
        m_yawVelocity = delta.x() * 0.0006;
        m_pitchVelocity = -delta.y() * 0.00046;
        rebuildProjectedStars();
    }

    m_hoveredEntryId = pickStar(event->position());
    m_lastMousePos = event->position();
    update();
    QWidget::mouseMoveEvent(event);
}

void StarMapView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragMoved = false;
        m_pressPos = event->position();
        m_lastMousePos = event->position();
    }
    QWidget::mousePressEvent(event);
}

void StarMapView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        if (!m_dragMoved && QLineF(m_pressPos, event->position()).length() < 8.0) {
            const auto hitId = pickStar(event->position());
            if (!hitId.isEmpty()) {
                emit entrySelected(hitId);
            }
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void StarMapView::leaveEvent(QEvent *event) {
    m_hoveredEntryId.clear();
    QWidget::leaveEvent(event);
}

void StarMapView::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    rebuildProjectedStars();
}

void StarMapView::initializeBackground() {
    m_backgroundStars.clear();
    m_backgroundStars.reserve(BackgroundStarCount + MilkyWayStarCount);

    for (int i = 0; i < BackgroundStarCount; ++i) {
        const qreal lon = QRandomGenerator::global()->generateDouble() * 2.0 * M_PI;
        const qreal lat = (QRandomGenerator::global()->generateDouble() - 0.5) * M_PI;

        BackgroundStar star;
        star.position = sphericalToCartesian(lon, lat);
        star.size = 0.35 + QRandomGenerator::global()->generateDouble() * 1.55;
        star.alpha = 0.15 + QRandomGenerator::global()->generateDouble() * 0.78;
        m_backgroundStars.append(star);
    }

    for (int i = 0; i < MilkyWayStarCount; ++i) {
        const qreal lon = -M_PI + (2.0 * M_PI * i / MilkyWayStarCount) +
                          (QRandomGenerator::global()->generateDouble() - 0.5) * 0.06;
        const qreal lat = qSin(lon * 1.18) * 0.2 +
                          (QRandomGenerator::global()->generateDouble() - 0.5) * 0.14;

        BackgroundStar star;
        star.position = sphericalToCartesian(lon, lat);
        star.size = 0.22 + QRandomGenerator::global()->generateDouble() * 1.15;
        star.alpha = 0.08 + QRandomGenerator::global()->generateDouble() * 0.3;
        m_backgroundStars.append(star);
    }
}

QVector3D StarMapView::rotatePoint(const QVector3D &point) const {
    const qreal cy = qCos(m_yaw);
    const qreal sy = qSin(m_yaw);
    const qreal cp = qCos(m_pitch);
    const qreal sp = qSin(m_pitch);

    QVector3D yawRotated(point.x() * cy + point.z() * sy, point.y(), -point.x() * sy + point.z() * cy);

    return QVector3D(
        yawRotated.x(),
        yawRotated.y() * cp - yawRotated.z() * sp,
        yawRotated.y() * sp + yawRotated.z() * cp);
}

QVector3D StarMapView::entryWorldPosition(int index, const EmotionEntry &entry) const {
    const AstroMeta meta = metaForEntry(entry);
    return sphericalToCartesian(meta.longitude + index * 0.036, meta.latitude);
}

bool StarMapView::projectPoint(const QVector3D &point, QPointF &screen, qreal &scale, qreal &depth) const {
    const QVector3D rotated = rotatePoint(point);
    if (rotated.z() <= -0.18) {
        return false;
    }

    const QPointF center(width() * 0.54, height() * 0.5);
    const qreal radius = qMin(width(), height()) * SphereRadiusFactor;
    const qreal perspective = 0.52 + (rotated.z() + 1.0) * 0.48;
    screen = QPointF(center.x() + rotated.x() * radius, center.y() - rotated.y() * radius);
    scale = perspective;
    depth = rotated.z();

    return QLineF(center, screen).length() <= radius + 1.0;
}

void StarMapView::rebuildProjectedStars() {
    m_projectedStars.clear();

    QList<EmotionEntry> sortedEntries = m_entries;
    std::sort(sortedEntries.begin(), sortedEntries.end(), [](const EmotionEntry &a, const EmotionEntry &b) {
        if (a.date == b.date) {
            return a.id < b.id;
        }
        return a.date < b.date;
    });

    for (int index = 0; index < sortedEntries.size(); ++index) {
        const auto &entry = sortedEntries.at(index);
        const AstroMeta meta = metaForEntry(entry);
        const QVector3D worldPos = entryWorldPosition(index, entry);

        QPointF screen;
        qreal scale = 0.0;
        qreal depth = 0.0;
        if (!projectPoint(worldPos, screen, scale, depth)) {
            continue;
        }

        ScreenStar star;
        star.id = entry.id;
        star.title = entry.title;
        star.emotion = entry.emotion;
        star.bagua = meta.bagua;
        star.ganzhi = meta.ganzhi;
        star.screenPos = screen;
        star.worldPos = worldPos;
        star.depth = depth;
        star.color = EmotionEntry::colorForEmotion(entry.emotion);
        star.radius = qBound<qreal>(1.5, (entry.intensity * 0.72 + 1.0) * (0.6 + scale * 0.4), 4.9);
        m_projectedStars.append(star);
    }
}

QString StarMapView::pickStar(const QPointF &pos) const {
    QString bestId;
    qreal bestDistance = 1e9;

    for (const auto &star : m_projectedStars) {
        const qreal distance = QLineF(pos, star.screenPos).length();
        if (distance <= star.radius * 2.35 && distance < bestDistance) {
            bestDistance = distance;
            bestId = star.id;
        }
    }
    return bestId;
}

void StarMapView::tickCamera() {
    if (!m_dragging) {
        m_yaw += m_yawVelocity;
        m_pitch = qBound(-MaxPitch, m_pitch + m_pitchVelocity, MaxPitch);
        m_yawVelocity *= 0.95;
        m_pitchVelocity *= 0.92;

        if (qAbs(m_yawVelocity) < 0.00002) {
            m_yawVelocity = 0.0;
        }
        if (qAbs(m_pitchVelocity) < 0.00002) {
            m_pitchVelocity = 0.0;
        }
    }

    if (m_dragging || qAbs(m_yawVelocity) > 0.0 || qAbs(m_pitchVelocity) > 0.0) {
        rebuildProjectedStars();
        update();
    }
}
