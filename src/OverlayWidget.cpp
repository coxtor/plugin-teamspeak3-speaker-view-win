#include "OverlayWidget.h"

#include <QtGui/QMoveEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QApplication>

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

    // WA_TranslucentBackground must be set BEFORE setWindowFlags, otherwise
    // Qt re-creates the native window without WS_EX_LAYERED and DWM won't
    // composite the alpha channel — on Windows this manifests as the
    // window disappearing entirely when toggling into frameless mode.
    setAttribute(Qt::WA_TranslucentBackground, m_borderless);
    setAutoFillBackground(!m_borderless);

    Qt::WindowFlags f = Qt::Tool | Qt::WindowDoesNotAcceptFocus;
    if (m_alwaysOnTop) f |= Qt::WindowStaysOnTopHint;
    if (m_borderless) f |= Qt::FramelessWindowHint;
    setWindowFlags(f);

    // Palette: dark HUD uses light text, normal tool window inherits host.
    // A default-constructed QPalette is all-black — not "reset to host".
    // Use the application palette explicitly for the non-HUD case.
    if (m_borderless) {
        QPalette pal;
        pal.setColor(QPalette::WindowText, QColor(240, 240, 240));
        pal.setColor(QPalette::Text,       QColor(240, 240, 240));
        pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(160, 160, 160));
        pal.setColor(QPalette::Disabled, QPalette::Text,       QColor(160, 160, 160));
        setPalette(pal);
    } else {
        setPalette(QApplication::palette());
    }

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
