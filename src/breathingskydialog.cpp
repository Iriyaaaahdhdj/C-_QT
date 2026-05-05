#include "breathingskydialog.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtMath>

namespace {
QColor sceneAccentForEmotion(const QString &emotion) {
    const QString value = EmotionEntry::normalizedEmotion(emotion);
    if (value == "快乐") return QColor("#ffd38d");
    if (value == "平静") return QColor("#c8deff");
    if (value == "难过") return QColor("#d6ddff");
    if (value == "愤怒") return QColor("#ffc8bb");
    if (value == "焦虑") return QColor("#decfff");
    return QColor("#d6efc4");
}

QString calmLineForEmotion(const QString &emotion) {
    const QString value = EmotionEntry::normalizedEmotion(emotion);
    if (value == "快乐") return "把这份亮光轻轻看清，再慢慢收进今晚。";
    if (value == "平静") return "让呼吸和星光一起放慢，继续待在这份安定里。";
    if (value == "难过") return "不用急着变好，先陪这颗星安静一会儿。";
    if (value == "愤怒") return "把绷紧的力气慢慢放下，让视线重新稳定。";
    if (value == "焦虑") return "先只管呼吸，星光会替你把慌张一点点放缓。";
    return "看着它，跟着它，一点点把今天放轻。";
}

class BreathingSceneWidget : public QWidget {
public:
    explicit BreathingSceneWidget(const EmotionEntry &entry, QWidget *parent = nullptr)
        : QWidget(parent), m_entry(entry) {
        setMinimumHeight(420);
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);
    }

    void setPhaseState(qreal breathValue, qreal progress, bool matched, bool completed) {
        m_breathValue = breathValue;
        m_progress = progress;
        m_matched = matched;
        m_completed = completed;
        update();
    }

    bool isHolding() const {
        return m_mouseHolding;
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_mouseHolding = true;
            update();
        }
        QWidget::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_mouseHolding = false;
            update();
        }
        QWidget::mouseReleaseEvent(event);
    }

    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        QLinearGradient bg(0, 0, 0, height());
        bg.setColorAt(0.0, QColor("#040917"));
        bg.setColorAt(0.45, QColor("#0a1530"));
        bg.setColorAt(1.0, QColor("#09101d"));
        painter.fillRect(rect(), bg);

        const QColor accent = sceneAccentForEmotion(m_entry.emotion);
        const QPointF starPos(width() * 0.76, height() * 0.22);
        const QPointF childPos(width() * 0.28, height() * 0.77);
        const qreal pulse = 0.74 + m_breathValue * 0.44;
        const qreal calmFactor = qBound<qreal>(0.0, m_progress, 1.0);
        const qreal jitterAmount = (1.0 - calmFactor) * (2.4 + m_entry.intensity * 0.5);
        const qreal jitterX = qSin(m_breathValue * 8.0 + m_progress * 3.5) * jitterAmount;
        const qreal jitterY = qCos(m_breathValue * 9.5 + m_progress * 4.1) * jitterAmount * 0.8;
        const QPointF stableStarPos = starPos + QPointF(jitterX, jitterY);

        painter.setPen(Qt::NoPen);
        for (int i = 0; i < 110; ++i) {
            const qreal x = (i * 53) % qMax(1, width());
            const qreal y = ((i * 97) + 40) % qMax(1, int(height() * 0.64));
            const qreal r = (i % 7 == 0) ? 1.7 : 1.0;
            QColor dust(236, 243, 255, (i % 5 == 0) ? 110 : 58);
            painter.setBrush(dust);
            painter.drawEllipse(QPointF(x, y), r, r);
        }

        QPainterPath hill;
        hill.moveTo(0, height());
        hill.cubicTo(width() * 0.12, height() * 0.68,
                     width() * 0.46, height() * 0.84,
                     width() * 0.66, height() * 0.72);
        hill.cubicTo(width() * 0.82, height() * 0.66,
                     width() * 0.94, height() * 0.84,
                     width(), height() * 0.76);
        hill.lineTo(width(), height());
        hill.closeSubpath();
        painter.fillPath(hill, QColor(6, 12, 19, 240));

        painter.setPen(QPen(QColor(255, 255, 255, 24), 1.0));
        painter.drawLine(childPos + QPointF(34, -26), stableStarPos);

        QRadialGradient halo(stableStarPos, 36 + pulse * 16);
        QColor glow = accent;
        glow.setAlpha(120);
        halo.setColorAt(0.0, glow);
        glow.setAlpha(0);
        halo.setColorAt(1.0, glow);
        painter.setBrush(halo);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(stableStarPos, 36 + pulse * 16, 36 + pulse * 16);

        painter.setBrush(QColor(255, 251, 246, 245));
        painter.drawEllipse(stableStarPos, 4.2 + m_entry.intensity * 0.45, 4.2 + m_entry.intensity * 0.45);
        painter.setBrush(accent);
        painter.drawEllipse(stableStarPos, 1.9, 1.9);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(m_matched ? QColor(255, 232, 177, 168) : QColor(188, 204, 235, 112), 1.8));
        painter.drawEllipse(stableStarPos, 20 + pulse * 18, 20 + pulse * 18);
        painter.setPen(QPen(QColor(255, 255, 255, 54), 1.0, Qt::DashLine));
        painter.drawEllipse(stableStarPos, 34, 34);

        painter.setBrush(QColor(13, 18, 28, 230));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(childPos + QPointF(0, -54), 15, 15);

        QPainterPath body;
        body.moveTo(childPos.x() - 24, childPos.y() + 34);
        body.quadTo(childPos.x() - 4, childPos.y() - 26, childPos.x() + 18, childPos.y() + 34);
        body.closeSubpath();
        painter.fillPath(body, QColor(12, 17, 27, 238));

        painter.setPen(QPen(QColor(12, 17, 27, 238), 7.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(childPos + QPointF(-9, 34), childPos + QPointF(-16, 72));
        painter.drawLine(childPos + QPointF(10, 34), childPos + QPointF(18, 72));
        painter.drawLine(childPos + QPointF(14, -28), childPos + QPointF(42, -10));

        painter.setPen(QPen(QColor(34, 40, 56, 248), 6.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(childPos + QPointF(36, -16), childPos + QPointF(68, -34));
        painter.setPen(QPen(QColor(42, 48, 64, 255), 5.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(childPos + QPointF(60, -26), childPos + QPointF(72, 16));
        painter.drawLine(childPos + QPointF(60, -26), childPos + QPointF(47, 10));
        painter.drawLine(childPos + QPointF(60, -26), childPos + QPointF(76, 10));

        const QRectF panelRect(20, 18, qMin(width() * 0.46, 340.0), 94);
        QPainterPath panel;
        panel.addRoundedRect(panelRect, 18, 18);
        painter.fillPath(panel, QColor(6, 12, 23, 170));
        painter.setPen(QColor("#f4f8ff"));
        painter.drawText(panelRect.adjusted(14, 12, -14, -56), QStringLiteral("望远镜已经对准这颗情绪星"));
        painter.setPen(QColor("#c4d4ee"));
        painter.drawText(panelRect.adjusted(14, 38, -14, -32), calmLineForEmotion(m_entry.emotion));
        painter.setPen(QColor("#8fa4c6"));
        painter.drawText(panelRect.adjusted(14, 62, -14, -10),
                         m_completed ? QStringLiteral("这颗星已经慢慢稳定下来。")
                                     : QStringLiteral("按住鼠标或空格吸气，松开呼气。"));

        const QRectF progressRect(width() * 0.18, height() - 42, width() * 0.64, 10);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 24));
        painter.drawRoundedRect(progressRect, 5, 5);
        QRectF fillRect = progressRect;
        fillRect.setWidth(progressRect.width() * calmFactor);
        painter.setBrush(m_completed ? QColor("#ffe3a8") : accent);
        painter.drawRoundedRect(fillRect, 5, 5);
    }

private:
    EmotionEntry m_entry;
    qreal m_breathValue = 0.0;
    qreal m_progress = 0.0;
    bool m_matched = false;
    bool m_completed = false;
    bool m_mouseHolding = false;
};
}

BreathingSkyDialog::BreathingSkyDialog(const EmotionEntry &entry, QWidget *parent)
    : QDialog(parent),
      m_entry(entry) {
    setWindowTitle("呼吸观星");
    resize(960, 720);
    setModal(true);
    setFocusPolicy(Qt::StrongFocus);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(14);

    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(12);

    auto *titleWrap = new QVBoxLayout();
    titleWrap->setSpacing(4);
    auto *title = new QLabel("呼吸观星", this);
    title->setObjectName("breatheTitle");
    auto *subtitle = new QLabel(QString("你正在看着“%1”这颗情绪星。").arg(entry.title), this);
    subtitle->setObjectName("breatheSubtitle");
    titleWrap->addWidget(title);
    titleWrap->addWidget(subtitle);

    m_phaseLabel = new QLabel(this);
    m_phaseLabel->setObjectName("phaseBadge");
    m_phaseLabel->setAlignment(Qt::AlignCenter);
    m_phaseLabel->setMinimumWidth(124);

    m_closeButton = new QPushButton("返回星空", this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);

    topRow->addLayout(titleWrap, 1);
    topRow->addWidget(m_phaseLabel);
    topRow->addWidget(m_closeButton);

    m_scene = new BreathingSceneWidget(entry, this);

    auto *bottomCard = new QWidget(this);
    bottomCard->setObjectName("bottomCard");
    auto *bottomLayout = new QHBoxLayout(bottomCard);
    bottomLayout->setContentsMargins(16, 14, 16, 14);
    bottomLayout->setSpacing(16);

    m_hintLabel = new QLabel(bottomCard);
    m_hintLabel->setObjectName("hintLabel");
    m_hintLabel->setWordWrap(true);

    m_progressLabel = new QLabel(bottomCard);
    m_progressLabel->setObjectName("progressLabel");
    m_progressLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_progressLabel->setMinimumWidth(160);

    bottomLayout->addWidget(m_hintLabel, 1);
    bottomLayout->addWidget(m_progressLabel);

    root->addLayout(topRow);
    root->addWidget(m_scene, 1);
    root->addWidget(bottomCard);

    setStyleSheet(R"(
        QDialog { background: #0a1220; }
        QLabel { color: #eef4ff; }
        QLabel#breatheTitle { font-size: 26px; font-weight: 700; }
        QLabel#breatheSubtitle { font-size: 13px; color: #9eb1d3; }
        QLabel#phaseBadge {
            background: rgba(255,255,255,0.08);
            border: 1px solid rgba(255,255,255,0.14);
            border-radius: 18px;
            padding: 10px 14px;
            font-size: 13px;
            font-weight: 700;
        }
        QWidget#bottomCard {
            background: rgba(255,255,255,0.04);
            border: 1px solid rgba(67,88,128,0.56);
            border-radius: 20px;
        }
        QLabel#hintLabel { font-size: 13px; color: #d5e2fa; line-height: 1.5; }
        QLabel#progressLabel { font-size: 13px; font-weight: 700; color: #ffe4b2; }
        QPushButton {
            background: rgba(244,248,255,0.1);
            color: white;
            border-radius: 16px;
            padding: 11px 16px;
            border: 1px solid rgba(255,255,255,0.12);
            font-weight: 700;
        }
        QPushButton:hover { background: rgba(244,248,255,0.16); }
    )");

    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, &BreathingSkyDialog::advanceAnimation);
    m_timer.start();
    rebuildStatus();
}

void BreathingSkyDialog::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceHolding = true;
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void BreathingSkyDialog::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceHolding = false;
        event->accept();
        return;
    }
    QDialog::keyReleaseEvent(event);
}

void BreathingSkyDialog::advanceAnimation() {
    auto *scene = static_cast<BreathingSceneWidget *>(m_scene);
    const bool holding = m_spaceHolding || scene->isHolding();
    constexpr qreal delta = 0.016;

    const qreal inhaleDuration = 3.8;
    const qreal holdDuration = 1.2;
    const qreal exhaleDuration = 4.2;
    const qreal restDuration = 0.9;

    if (m_phase != BreathPhase::Complete) {
        m_phaseElapsed += delta;
    }

    qreal breathValue = 0.0;
    bool matched = false;

    switch (m_phase) {
    case BreathPhase::Inhale:
        breathValue = qBound<qreal>(0.0, m_phaseElapsed / inhaleDuration, 1.0);
        matched = holding;
        if (m_phaseElapsed >= inhaleDuration) {
            m_phase = BreathPhase::Hold;
            m_phaseElapsed = 0.0;
        }
        break;
    case BreathPhase::Hold:
        breathValue = 1.0;
        matched = holding;
        if (m_phaseElapsed >= holdDuration) {
            m_phase = BreathPhase::Exhale;
            m_phaseElapsed = 0.0;
        }
        break;
    case BreathPhase::Exhale:
        breathValue = qBound<qreal>(0.0, 1.0 - (m_phaseElapsed / exhaleDuration), 1.0);
        matched = !holding;
        if (m_phaseElapsed >= exhaleDuration) {
            m_phase = BreathPhase::Rest;
            m_phaseElapsed = 0.0;
        }
        break;
    case BreathPhase::Rest:
        breathValue = 0.0;
        matched = !holding;
        if (m_phaseElapsed >= restDuration) {
            m_phase = BreathPhase::Inhale;
            m_phaseElapsed = 0.0;
        }
        break;
    case BreathPhase::Complete:
        breathValue = 0.65;
        matched = true;
        break;
    }

    if (m_phase != BreathPhase::Complete) {
        if (matched) {
            m_progress = qMin<qreal>(1.0, m_progress + delta * 0.085);
        } else {
            m_progress = qMax<qreal>(0.0, m_progress - delta * 0.03);
        }
        if (m_progress >= 1.0) {
            m_phase = BreathPhase::Complete;
            m_phaseElapsed = 0.0;
            m_timer.stop();
        }
    }

    scene->setPhaseState(breathValue, m_progress, matched, m_phase == BreathPhase::Complete);
    rebuildStatus();
}

void BreathingSkyDialog::rebuildStatus() {
    m_phaseLabel->setText(phaseTitle());
    m_hintLabel->setText(phaseGuide());
    if (m_phase == BreathPhase::Complete) {
        m_progressLabel->setText("已稳定 100%");
    } else {
        m_progressLabel->setText(QString("平稳度 %1%").arg(int(m_progress * 100.0)));
    }
}

QString BreathingSkyDialog::phaseTitle() const {
    switch (m_phase) {
    case BreathPhase::Inhale:
        return "吸气";
    case BreathPhase::Hold:
        return "停留";
    case BreathPhase::Exhale:
        return "呼气";
    case BreathPhase::Rest:
        return "放松";
    case BreathPhase::Complete:
        return "已安定";
    }
    return QString();
}

QString BreathingSkyDialog::phaseGuide() const {
    switch (m_phase) {
    case BreathPhase::Inhale:
        return "按住鼠标或空格，跟着这颗星慢慢吸气，让视线一点点对准它。";
    case BreathPhase::Hold:
        return "继续轻轻按住，停一会儿，让星光先稳定下来。";
    case BreathPhase::Exhale:
        return "慢慢松开，跟着它呼气，把紧绷感一点点放下。";
    case BreathPhase::Rest:
        return "什么都不用做，短暂放松一下，再开始下一轮呼吸。";
    case BreathPhase::Complete:
        return "这颗情绪星已经被你温柔地安放好了。准备好了就回到主星空。";
    }
    return QString();
}
