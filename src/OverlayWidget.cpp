#include "OverlayWidget.h"

#include "NativeStyle.h"

#include <QtGui/QMoveEvent>
#include <QtGui/QResizeEvent>

OverlayWidget::OverlayWidget(QWidget* parent) : QWidget(parent) {
    // TS3 forces its dark Fusion palette on every widget globally. Override
    // on our top-level windows so Speaker View looks like a normal Windows
    // app instead of inheriting the host's dark chrome.
    applyNativeWindowsLook(this);

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

void OverlayWidget::applyWindowFlags() {
    const bool visible = isVisible();
    const QRect geo = geometry();

    Qt::WindowFlags f = Qt::Tool | Qt::WindowDoesNotAcceptFocus;
    if (m_alwaysOnTop) f |= Qt::WindowStaysOnTopHint;
    setWindowFlags(f);

    if (geo.isValid()) setGeometry(geo);
    if (visible) show();
}

void OverlayWidget::moveEvent(QMoveEvent* event) {
    QWidget::moveEvent(event);
    emit frameChanged();
}

void OverlayWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    emit frameChanged();
}
