#include "OverlayWidget.h"

#include <QtGui/QMoveEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

OverlayWidget::OverlayWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setWindowTitle(QStringLiteral("Speaker View"));
    resize(260, 120);

    // Window flags are set ONCE, in the constructor. Changing them later via
    // setWindowFlags() destroys and recreates the native HWND — which is
    // unreliable with children that own QGraphicsOpacityEffect and running
    // QPropertyAnimation (symptom: TS3 crashes on toggling always-on-top).
    // Runtime toggles (always-on-top, click-through) go through the Win32
    // APIs below instead so the HWND stays alive.
    setWindowFlags(Qt::Tool | Qt::WindowDoesNotAcceptFocus);
    applyAlwaysOnTopNative();
    applyClickThroughNative();
}

void OverlayWidget::setAlwaysOnTop(bool on) {
    if (m_alwaysOnTop == on) return;
    m_alwaysOnTop = on;
    applyAlwaysOnTopNative();
}

void OverlayWidget::setClickThrough(bool on) {
    if (m_clickThrough == on) return;
    m_clickThrough = on;
    setAttribute(Qt::WA_TransparentForMouseEvents, on);
    applyClickThroughNative();
}

void OverlayWidget::applyAlwaysOnTopNative() {
#ifdef _WIN32
    if (!windowHandle()) return;  // not yet mapped; native apply after show()
    HWND hwnd = reinterpret_cast<HWND>(winId());
    ::SetWindowPos(hwnd,
                   m_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                   0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
}

void OverlayWidget::applyClickThroughNative() {
#ifdef _WIN32
    if (!windowHandle()) return;
    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG_PTR ex = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (m_clickThrough) {
        ex |= (WS_EX_LAYERED | WS_EX_TRANSPARENT);
    } else {
        ex &= ~WS_EX_TRANSPARENT;
        // Leave WS_EX_LAYERED; removing it while the window is live is
        // what causes it to blink out.
    }
    ::SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
#endif
}

void OverlayWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // The native HWND only exists after the first show(); re-apply so our
    // constructor-time intent is realised.
    applyAlwaysOnTopNative();
    applyClickThroughNative();
}

void OverlayWidget::moveEvent(QMoveEvent* event) {
    QWidget::moveEvent(event);
    emit frameChanged();
}

void OverlayWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    emit frameChanged();
}
