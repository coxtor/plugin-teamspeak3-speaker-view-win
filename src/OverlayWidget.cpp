#include "OverlayWidget.h"

#include <QtGui/QMoveEvent>
#include <QtGui/QResizeEvent>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

OverlayWidget::OverlayWidget(QWidget* parent) : QWidget(parent) {
    // IMPORTANT: keep the exact construction order from v0.2
    // (setWindowTitle + resize BEFORE applyWindowFlags). Reordering
    // these on Qt 5.15 leaves the window invisible.
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setWindowTitle(QStringLiteral("Speaker View"));
    resize(260, 120);
    applyWindowFlags();
}

void OverlayWidget::setAlwaysOnTop(bool on) {
    if (m_alwaysOnTop == on) return;
    m_alwaysOnTop = on;
    // NOT applyWindowFlags(): setWindowFlags() rebuilds the native HWND
    // which crashes TS3 when child widgets own running QGraphicsEffect
    // animations. Touch only the Win32 Z-order flag on the live window.
    applyAlwaysOnTopNative();
}

void OverlayWidget::setClickThrough(bool on) {
    if (m_clickThrough == on) return;
    m_clickThrough = on;
    setAttribute(Qt::WA_TransparentForMouseEvents, on);
    // Same reason as above: go native instead of rebuilding the HWND.
    applyClickThroughNative();
}

void OverlayWidget::applyWindowFlags() {
    // Constructor-only: sets the full initial flag set in one shot.
    // Runtime toggles use the native helpers below to avoid HWND rebuild.
    Qt::WindowFlags f = Qt::Tool | Qt::WindowDoesNotAcceptFocus;
    if (m_alwaysOnTop) f |= Qt::WindowStaysOnTopHint;
    setWindowFlags(f);
}

void OverlayWidget::applyAlwaysOnTopNative() {
#ifdef _WIN32
    if (!isVisible()) return;  // not yet shown; Qt flags cover the initial state
    HWND hwnd = reinterpret_cast<HWND>(winId());
    ::SetWindowPos(hwnd,
                   m_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                   0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
}

void OverlayWidget::applyClickThroughNative() {
#ifdef _WIN32
    if (!isVisible()) return;
    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG_PTR ex = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (m_clickThrough) {
        ex |= (WS_EX_LAYERED | WS_EX_TRANSPARENT);
    } else {
        ex &= ~WS_EX_TRANSPARENT;
        // Leave WS_EX_LAYERED bit — removing it on a live window is what
        // makes the window blink out on some Windows builds.
    }
    ::SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
#endif
}

void OverlayWidget::moveEvent(QMoveEvent* event) {
    QWidget::moveEvent(event);
    emit frameChanged();
}

void OverlayWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    emit frameChanged();
}
