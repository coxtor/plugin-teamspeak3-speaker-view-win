#include "OverlayWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QMoveEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

OverlayWidget::OverlayWidget(QWidget* parent) : QWidget(parent) {
    // Initial window flags, set once — same flag set v0.2 used and which
    // was proven to produce a visible window. Runtime changes to these
    // flags via setWindowFlags() rebuild the native HWND and crash TS3
    // when children hold running QGraphicsEffect animations, so instead
    // runtime toggles go through Win32 APIs on the live HWND below.
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    Qt::WindowFlags f = Qt::Tool | Qt::WindowDoesNotAcceptFocus;
    if (m_alwaysOnTop) f |= Qt::WindowStaysOnTopHint;
    setWindowFlags(f);
    setWindowTitle(QStringLiteral("Speaker View"));
    resize(260, 120);
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
    // Defer native Win32 tweaks until AFTER Qt has finished its own
    // ShowWindow(SW_SHOW) sequence. Calling SetWindowPos(HWND_TOPMOST,
    // SWP_NOACTIVATE) synchronously during the first showEvent races with
    // Qt's internal show path on 5.15 Windows and the window ends up
    // created but never actually shown to the user.
    QTimer::singleShot(0, this, [this]() {
        applyAlwaysOnTopNative();
        applyClickThroughNative();
    });
}

void OverlayWidget::moveEvent(QMoveEvent* event) {
    QWidget::moveEvent(event);
    emit frameChanged();
}

void OverlayWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    emit frameChanged();
}
