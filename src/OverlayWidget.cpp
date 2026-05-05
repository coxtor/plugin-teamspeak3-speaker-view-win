#include "OverlayWidget.h"

#include "Log.h"

#include <QtGui/QHideEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QMoveEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/QScreen>
#include <QtGui/QShowEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

#ifdef _WIN32
// WIN32_LEAN_AND_MEAN is already defined as a compile definition in CMake.
#  include <windows.h>
#endif

namespace {
constexpr int kTitleBarHeight = 24;
}

OverlayWidget::OverlayWidget(QWidget* parent) : QWidget(parent) {
    sv_log("OverlayWidget ctor: enter");
    // IMPORTANT: keep the exact construction order from v0.2
    // (setWindowTitle + resize BEFORE applyWindowFlags). Reordering
    // these on Qt 5.15 leaves the window invisible.
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setWindowTitle(QStringLiteral("Gesprächsansicht"));
    resize(260, 140);
    applyWindowFlags();
    buildChrome();
    sv_log(QStringLiteral("OverlayWidget ctor: exit. flags=0x%1")
           .arg(static_cast<quint32>(windowFlags()), 0, 16));
}

void OverlayWidget::buildChrome() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---- Title bar ----
    m_titleBar = new QWidget(this);
    m_titleBar->setFixedHeight(kTitleBarHeight);
    m_titleBar->setAutoFillBackground(true);
    {
        QPalette p = m_titleBar->palette();
        p.setColor(QPalette::Window, QColor(45, 45, 48));
        p.setColor(QPalette::WindowText, QColor(240, 240, 240));
        m_titleBar->setPalette(p);
    }

    auto* titleRow = new QHBoxLayout(m_titleBar);
    titleRow->setContentsMargins(8, 0, 0, 0);
    titleRow->setSpacing(0);

    m_titleLabel = new QLabel(QStringLiteral("Gesprächsansicht"), m_titleBar);
    {
        QFont f = m_titleLabel->font();
        f.setPointSizeF(f.pointSizeF() - 0.5);
        f.setWeight(QFont::Medium);
        m_titleLabel->setFont(f);
        QPalette p = m_titleLabel->palette();
        p.setColor(QPalette::WindowText, QColor(240, 240, 240));
        m_titleLabel->setPalette(p);
    }
    titleRow->addWidget(m_titleLabel, /*stretch*/ 1);

    auto* closeBtn = new QToolButton(m_titleBar);
    closeBtn->setText(QStringLiteral("×"));
    closeBtn->setFixedSize(kTitleBarHeight + 16, kTitleBarHeight);
    closeBtn->setCursor(Qt::ArrowCursor);
    closeBtn->setFocusPolicy(Qt::NoFocus);
    closeBtn->setAutoRaise(true);
    closeBtn->setStyleSheet(
        QStringLiteral(
            "QToolButton { color: #F0F0F0; background: transparent; border: none;"
            "              font-size: 16px; font-weight: bold; }"
            "QToolButton:hover   { background: #E81123; color: white; }"
            "QToolButton:pressed { background: #C50F1F; color: white; }"));
    connect(closeBtn, &QToolButton::clicked, this, &QWidget::hide);
    titleRow->addWidget(closeBtn, /*stretch*/ 0);

    root->addWidget(m_titleBar);

    // ---- Content area ----
    auto* content = new QWidget(this);
    m_contentLayout = new QVBoxLayout(content);
    m_contentLayout->setContentsMargins(6, 6, 6, 6);
    m_contentLayout->setSpacing(4);
    m_contentLayout->addStretch();
    root->addWidget(content, /*stretch*/ 1);
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
    Qt::WindowFlags f = Qt::Tool | Qt::WindowDoesNotAcceptFocus
                      | Qt::FramelessWindowHint;
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

void OverlayWidget::paintEvent(QPaintEvent* /*event*/) {
    // Paint the content-area background + a subtle border. Title bar
    // paints itself via autoFillBackground=true on a dark palette.
    QPainter p(this);
    const QRect contentRect = rect().adjusted(0, kTitleBarHeight, 0, 0);
    p.fillRect(contentRect, QColor(32, 32, 32));

    QPen border(QColor(90, 90, 90));
    border.setWidth(1);
    p.setPen(border);
    // Inset by half a pen width so the 1px line lands on integer pixels.
    p.drawRect(rect().adjusted(0, 0, -1, -1));
}

void OverlayWidget::mousePressEvent(QMouseEvent* event) {
    // Drag only when the press lands on the title bar.
    if (event->button() == Qt::LeftButton
        && event->pos().y() < kTitleBarHeight) {
        m_dragging = true;
        m_dragOffset = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void OverlayWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragOffset);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void OverlayWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void OverlayWidget::showEvent(QShowEvent* event) {
    sv_log(QStringLiteral("OverlayWidget::showEvent. geom=%1,%2 %3x%4 screen=%5 winId=0x%6")
           .arg(x()).arg(y()).arg(width()).arg(height())
           .arg(screen() ? screen()->name() : QStringLiteral("none"))
           .arg(static_cast<quintptr>(winId()), 0, 16));
    QWidget::showEvent(event);
}

void OverlayWidget::hideEvent(QHideEvent* event) {
    sv_log(QStringLiteral("OverlayWidget::hideEvent. geom=%1,%2 %3x%4")
           .arg(x()).arg(y()).arg(width()).arg(height()));
    QWidget::hideEvent(event);
}

void OverlayWidget::moveEvent(QMoveEvent* event) {
    QWidget::moveEvent(event);
    emit frameChanged();
}

void OverlayWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    emit frameChanged();
}
