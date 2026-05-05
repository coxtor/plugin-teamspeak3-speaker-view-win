#pragma once

#include <QtCore/QPoint>
#include <QtWidgets/QWidget>

class QLabel;
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
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void applyWindowFlags();            // constructor-only
    void applyAlwaysOnTopNative();      // runtime toggle
    void applyClickThroughNative();     // runtime toggle
    void buildChrome();                 // title bar + content area

    QWidget*     m_titleBar = nullptr;
    QLabel*      m_titleLabel = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;

    bool   m_alwaysOnTop = true;
    bool   m_clickThrough = false;
    bool   m_dragging = false;
    QPoint m_dragOffset;
};
