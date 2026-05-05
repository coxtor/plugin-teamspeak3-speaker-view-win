#include "OverlayWidget.h"

#include <QtGui/QMoveEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

OverlayWidget::OverlayWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_ShowWithoutActivating, true);
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
    if (m_borderless) f |= Qt::FramelessWindowHint;
    setWindowFlags(f);

    // Translucent background is only composited by DWM for frameless windows.
    // If we set WA_TranslucentBackground on a normal tool window, Windows
    // renders the client area as opaque black — and our paintEvent's
    // semi-transparent overlay on top of that gives a solid black box.
    setAttribute(Qt::WA_TranslucentBackground, m_borderless);
    setAutoFillBackground(!m_borderless);

    if (geo.isValid()) setGeometry(geo);
    if (visible) show();
    update();
}

void OverlayWidget::paintEvent(QPaintEvent* event) {
    if (!m_borderless) {
        // Normal tool window: let Qt paint its native background; do nothing.
        QWidget::paintEvent(event);
        return;
    }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor bg(20, 20, 20, 200);  // semi-transparent dark HUD, only when frameless
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
