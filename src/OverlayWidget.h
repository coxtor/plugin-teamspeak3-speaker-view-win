#pragma once

#include <QtCore/QPoint>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

class QLabel;
class QToolButton;
class QVBoxLayout;

// Top-level overlay window. Frameless, with a custom title bar (title +
// close button + drag-to-move). Frameless is set once in the ctor; do
// NOT toggle frameless at runtime — changing window flags on a live
// widget destroys and re-creates the native HWND and crashes TS3 when
// children hold running graphics effects.
class OverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget* parent = nullptr);

    void setAlwaysOnTop(bool on);
    void setClickThrough(bool on);

    // Layout clients add their row widgets here, not to the top-level
    // layout — otherwise they'd overlap the title bar.
    QVBoxLayout* contentLayout() const { return m_contentLayout; }

signals:
    void frameChanged();

protected:
    bool event(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
#ifdef _WIN32
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
#endif

private:
    void applyWindowFlags();            // constructor-only
    void applyAlwaysOnTopNative();      // runtime toggle
    void applyClickThroughNative();     // runtime toggle
    void buildChrome();                 // title bar + content area
    void applyTheme();                  // (re)paint palette into chrome

    QWidget*     m_titleBar = nullptr;
    QLabel*      m_titleLabel = nullptr;
    QToolButton* m_closeBtn = nullptr;
    QWidget*     m_contentPane = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;

    QColor m_contentBg;
    QColor m_border;

    bool   m_alwaysOnTop = true;
    bool   m_clickThrough = false;
    bool   m_dragging = false;
    QPoint m_dragOffset;
};
