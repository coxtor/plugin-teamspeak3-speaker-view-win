#include "OverlayWidget.h"

#include <QtGui/QMoveEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

OverlayWidget::OverlayWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAutoFillBackground(false);
    setWindowTitle(QStringLiteral("Speaker View"));
    resize(260, 120);
    applyWindowFlags();
}

void OverlayWidget::setAlwaysOnTop(bool on) {
    if (m_alwaysOnTop == on) return;
    m_alwaysOnTop = on;
    applyWindowFlags();
}

void OverlayWidget::setClickThrough(bool on) {
    if (m_clickThrough == on) return;
    m_clickThrough = on;
    setAttribute(Qt::WA_TransparentForMouseEvents, on);
    applyWindowFlags();
}

void OverlayWidget::setBorderless(bool on) {
    if (m_borderless == on) return;
    m_borderless = on;
    applyWindowFlags();
}

void OverlayWidget::applyWindowFlags() {
    const bool visible = isVisible();
    const QRect geo = geometry();

    Qt::WindowFlags f = Qt::Tool | Qt::WindowDoesNotAcceptFocus;
    if (m_alwaysOnTop) f |= Qt::WindowStaysOnTopHint;
    if (m_borderless) {
        f |= Qt::FramelessWindowHint;
    } else {
        // A normal tool window still has its small title bar + close button.
    }
    setWindowFlags(f);

    if (geo.isValid()) setGeometry(geo);
    if (visible) show();
}

void OverlayWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor bg(20, 20, 20, 200);  // semi-transparent dark, HUD-like
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 8, 8);
}

void OverlayWidget::moveEvent(QMoveEvent* event) {
    QWidget::moveEvent(event);
    emit frameChanged();
}

void OverlayWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    emit frameChanged();
}
