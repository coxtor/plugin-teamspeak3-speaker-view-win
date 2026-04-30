#include "SpeakerRowWidget.h"

#include <QtCore/QPropertyAnimation>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsOpacityEffect>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

SpeakerRowWidget::SpeakerRowWidget(const SpeakerSnapshot& snapshot,
                                   bool showAvatar, bool showChannel,
                                   QWidget* parent)
    : QWidget(parent), m_clientID(snapshot.clientID) {
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAutoFillBackground(false);
    setFixedHeight(28);

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(18, 2, 8, 2);
    outer->setSpacing(8);

    auto* textColumn = new QVBoxLayout();
    textColumn->setContentsMargins(0, 0, 0, 0);
    textColumn->setSpacing(0);

    m_nickLabel = new QLabel(this);
    QFont f = m_nickLabel->font();
    f.setPointSizeF(f.pointSizeF() + 1.0);
    f.setWeight(QFont::Medium);
    m_nickLabel->setFont(f);
    m_nickLabel->setStyleSheet("color: #F0F0F0;");
    textColumn->addWidget(m_nickLabel);

    m_channelLabel = new QLabel(this);
    QFont sf = m_channelLabel->font();
    sf.setPointSizeF(std::max(7.0, sf.pointSizeF() - 2.0));
    m_channelLabel->setFont(sf);
    m_channelLabel->setStyleSheet("color: #A0A0A0;");
    textColumn->addWidget(m_channelLabel);

    outer->addLayout(textColumn);
    outer->addStretch();

    m_opacity = new QGraphicsOpacityEffect(this);
    m_opacity->setOpacity(1.0);
    setGraphicsEffect(m_opacity);

    updateSnapshot(snapshot, showAvatar, showChannel, 0.0);
    (void)showAvatar;  // avatars are a future follow-up item (see tasks/todo.md)
}

void SpeakerRowWidget::updateSnapshot(const SpeakerSnapshot& s, bool /*showAvatar*/,
                                      bool showChannel, double fadeDuration) {
    m_clientID = s.clientID;
    m_talking = s.talking;

    m_nickLabel->setText(s.nickname);
    m_channelLabel->setVisible(showChannel && !s.channelName.isEmpty());
    m_channelLabel->setText(s.channelName);

    double target = 1.0;
    if (!s.talking && fadeDuration > 0.0) {
        target = std::clamp(s.remainingFadeSeconds / fadeDuration, 0.25, 1.0);
    }
    auto* anim = new QPropertyAnimation(m_opacity, "opacity", this);
    anim->setDuration(200);
    anim->setStartValue(m_opacity->opacity());
    anim->setEndValue(target);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    update();  // repaint the status dot
}

void SpeakerRowWidget::animateRemoval(int durationMs, std::function<void()> completion) {
    auto* anim = new QPropertyAnimation(m_opacity, "opacity", this);
    anim->setDuration(std::max(150, durationMs));
    anim->setStartValue(m_opacity->opacity());
    anim->setEndValue(0.0);
    if (completion) {
        QObject::connect(anim, &QPropertyAnimation::finished, this,
                         [completion]() { completion(); });
    }
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void SpeakerRowWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor c = m_talking ? QColor(52, 199, 89) : QColor(120, 120, 120);
    p.setBrush(c);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(8.0, height() / 2.0), 4.0, 4.0);
}
