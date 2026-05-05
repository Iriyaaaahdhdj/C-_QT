#pragma once

#include <QDialog>
#include <QTimer>

#include "emotionentry.h"

class QKeyEvent;
class QLabel;
class QPushButton;
class QWidget;

class BreathingSkyDialog : public QDialog {
    Q_OBJECT

public:
    explicit BreathingSkyDialog(const EmotionEntry &entry, QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    enum class BreathPhase {
        Inhale,
        Hold,
        Exhale,
        Rest,
        Complete
    };

    void advanceAnimation();
    void rebuildStatus();
    QString phaseTitle() const;
    QString phaseGuide() const;

    EmotionEntry m_entry;
    QWidget *m_scene = nullptr;
    QLabel *m_phaseLabel = nullptr;
    QLabel *m_hintLabel = nullptr;
    QLabel *m_progressLabel = nullptr;
    QPushButton *m_closeButton = nullptr;
    QTimer m_timer;
    BreathPhase m_phase = BreathPhase::Inhale;
    qreal m_phaseElapsed = 0.0;
    qreal m_progress = 0.0;
    bool m_spaceHolding = false;
};
