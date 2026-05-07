#include "OverlayWidget.h"

#include "Log.h"
#include "Theme.h"

#include <QtCore/QEvent>
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
#  include <dwmapi.h>

// Windows 11 rounded-corner API. Fall back to numeric constants when
// compiling against an older SDK that doesn't define these yet — DWM
// will ignore unknown attribute IDs at runtime.
#  ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#    define DWMWA_WINDOW_CORNER_PREFERENCE 33
#  endif
#  ifndef DWMWCP_ROUND
#    define DWMWCP_ROUND 2
#  endif
#endif

namespace {
constexpr int kTitleBarHeight = 24;

// Ask DWM to draw the usual Windows drop-shadow around our frameless
// window, and (on Windows 11) to round the outer corners. DWM only
// draws the shadow for layered/frameless windows that claim a non-
// client area — the classic trick is to extend the frame by one pixel
// into the client area with DwmExtendFrameIntoClientArea, then tell
// DWM via CSD that the window has a one-pixel "caption" region. No
// visual change to the client — DWM paints only the shadow.
void applyNativeWindowEffects(HWND hwnd) {
#ifdef _WIN32
    if (!hwnd) return;
    const MARGINS m{ 1, 1, 1, 1 };
    ::DwmExtendFrameIntoClientArea(hwnd, &m);

    const DWORD policy = 2;  // DWMNCRP_ENABLED — force shadow on
    ::DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY,
                            &policy, sizeof(policy));

    // Rounded corners: DWM 10.0.22000+ silently accepts the attribute,
    // older Windows returns an error we don't care about.
    const int corner = DWMWCP_ROUND;
    ::DwmSetWindowAttribute(hwnd,
                            static_cast<DWORD>(DWMWA_WINDOW_CORNER_PREFERENCE),
                            &corner, sizeof(corner));
#else
    (void)hwnd;
#endif
}

struct Palette {
    QColor titleBg;
    QColor titleText;
    QColor contentBg;
    QColor contentText;
    QColor border;
    QColor closeHover;
    QColor closePress;
    QColor closeGlyph;
};

Palette paletteForTheme(bool dark) {
    Palette p;
    // Close button hover/press match the OS-standard close glyph colors
    // (that red is the same on light + dark Windows 11 title bars).
    p.closeHover = QColor(0xE8, 0x11, 0x23);
    p.closePress = QColor(0xC5, 0x0F, 0x1F);
    if (dark) {
        p.titleBg     = QColor(0x2D, 0x2D, 0x30);
        p.titleText   = QColor(0xF0, 0xF0, 0xF0);
        p.contentBg   = QColor(0x20, 0x20, 0x20);
        p.contentText = QColor(0xF0, 0xF0, 0xF0);
        p.border      = QColor(0x5A, 0x5A, 0x5A);
        p.closeGlyph  = QColor(0xF0, 0xF0, 0xF0);
    } else {
        p.titleBg     = QColor(0xF0, 0xF0, 0xF0);
        p.titleText   = QColor(0x1F, 0x1F, 0x1F);
        p.contentBg   = QColor(0xFF, 0xFF, 0xFF);
        p.contentText = QColor(0x1F, 0x1F, 0x1F);
        p.border      = QColor(0xC8, 0xC8, 0xC8);
        p.closeGlyph  = QColor(0x1F, 0x1F, 0x1F);
    }
    return p;
}
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

    auto* titleRow = new QHBoxLayout(m_titleBar);
    titleRow->setContentsMargins(8, 0, 0, 0);
    titleRow->setSpacing(0);

    m_titleLabel = new QLabel(QStringLiteral("Gesprächsansicht"), m_titleBar);
    {
        QFont f = m_titleLabel->font();
        f.setPointSizeF(f.pointSizeF() - 0.5);
        f.setWeight(QFont::Medium);
        m_titleLabel->setFont(f);
    }
    titleRow->addWidget(m_titleLabel, /*stretch*/ 1);

    m_closeBtn = new QToolButton(m_titleBar);
    m_closeBtn->setText(QStringLiteral("×"));
    m_closeBtn->setFixedSize(kTitleBarHeight + 16, kTitleBarHeight);
    m_closeBtn->setCursor(Qt::ArrowCursor);
    m_closeBtn->setFocusPolicy(Qt::NoFocus);
    m_closeBtn->setAutoRaise(true);
    connect(m_closeBtn, &QToolButton::clicked, this, &QWidget::hide);
    titleRow->addWidget(m_closeBtn, /*stretch*/ 0);

    root->addWidget(m_titleBar);

    // ---- Content area ----
    m_contentPane = new QWidget(this);
    m_contentPane->setAutoFillBackground(true);
    m_contentLayout = new QVBoxLayout(m_contentPane);
    m_contentLayout->setContentsMargins(6, 6, 6, 6);
    m_contentLayout->setSpacing(4);
    m_contentLayout->addStretch();
    root->addWidget(m_contentPane, /*stretch*/ 1);

    applyTheme();
}

void OverlayWidget::applyTheme() {
    const Palette pal = paletteForTheme(sv_systemUsesDarkTheme());
    m_contentBg = pal.contentBg;
    m_border    = pal.border;

    if (m_titleBar) {
        QPalette p = m_titleBar->palette();
        p.setColor(QPalette::Window,     pal.titleBg);
        p.setColor(QPalette::WindowText, pal.titleText);
        m_titleBar->setPalette(p);
    }
    if (m_titleLabel) {
        QPalette p = m_titleLabel->palette();
        p.setColor(QPalette::WindowText, pal.titleText);
        m_titleLabel->setPalette(p);
    }
    if (m_contentPane) {
        QPalette p = m_contentPane->palette();
        p.setColor(QPalette::Window,     pal.contentBg);
        p.setColor(QPalette::WindowText, pal.contentText);
        // Text role so child QLabels that inherit the palette pick it up.
        p.setColor(QPalette::Text,       pal.contentText);
        m_contentPane->setPalette(p);
    }
    if (m_closeBtn) {
        const QString css = QStringLiteral(
            "QToolButton { color: %1; background: transparent; border: none;"
            "              font-size: 16px; font-weight: bold; }"
            "QToolButton:hover   { background: %2; color: white; }"
            "QToolButton:pressed { background: %3; color: white; }")
            .arg(pal.closeGlyph.name(),
                 pal.closeHover.name(),
                 pal.closePress.name());
        m_closeBtn->setStyleSheet(css);
    }
    update();
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
    // Title bar and content pane paint themselves via autoFillBackground.
    // On Windows, DWM paints the native drop-shadow + Win11 rounded
    // corners outside our client area (set up in showEvent), so we do
    // NOT draw any border here — it would otherwise fight the DWM rounding.
#ifndef _WIN32
    // Non-Windows fallback: keep a 1px outline so the frameless window
    // still has a visible edge.
    QPainter p(this);
    QPen border(m_border);
    border.setWidth(1);
    p.setPen(border);
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(0, 0, -1, -1));
#endif
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

bool OverlayWidget::event(QEvent* ev) {
    // Windows posts WM_SETTINGCHANGE on light/dark theme changes; Qt
    // surfaces these as ThemeChange / ApplicationPaletteChange. Re-apply
    // our custom chrome's palette when either fires.
    switch (ev->type()) {
    case QEvent::ThemeChange:
    case QEvent::ApplicationPaletteChange:
    case QEvent::PaletteChange:
        applyTheme();
        break;
    default:
        break;
    }
    return QWidget::event(ev);
}

void OverlayWidget::showEvent(QShowEvent* event) {
    sv_log(QStringLiteral("OverlayWidget::showEvent. geom=%1,%2 %3x%4 screen=%5 winId=0x%6")
           .arg(x()).arg(y()).arg(width()).arg(height())
           .arg(screen() ? screen()->name() : QStringLiteral("none"))
           .arg(static_cast<quintptr>(winId()), 0, 16));
    QWidget::showEvent(event);
#ifdef _WIN32
    applyNativeWindowEffects(reinterpret_cast<HWND>(winId()));
#endif
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
