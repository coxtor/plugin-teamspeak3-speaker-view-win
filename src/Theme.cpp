#include "Theme.h"

#include <QtGui/QColor>
#include <QtGui/QPalette>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

#ifdef _WIN32
#include <windows.h>
#endif

bool sv_systemUsesDarkTheme() {
#ifdef _WIN32
    DWORD lightMode = 1;  // default to light if the key is missing
    DWORD sz = sizeof(lightMode);
    HKEY key = nullptr;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &key) == ERROR_SUCCESS) {
        ::RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(&lightMode), &sz);
        ::RegCloseKey(key);
    }
    return lightMode == 0;
#else
    return false;
#endif
}

void sv_applyThemeToDialog(QWidget* root) {
    if (!root) return;
    if (!sv_systemUsesDarkTheme()) {
        // Light mode — let Qt's default palette stand. The host application
        // (TS3 Qt 5.15) already paints widgets correctly on a light system,
        // so we don't need to override anything.
        return;
    }

    // Dark mode: build a palette covering every role the dialog actually
    // touches. Without this, our QLabel / QPlainTextEdit / QSpinBox /
    // QGroupBox / QTabWidget render with black text on TS3's dark window
    // background — illegible.
    const QColor bgWindow   (0x2B, 0x2B, 0x2B);
    const QColor bgBase     (0x1E, 0x1E, 0x1E);  // text-entry backgrounds
    const QColor bgAlt      (0x32, 0x32, 0x32);  // alternate row, group box
    const QColor fgText     (0xE6, 0xE6, 0xE6);
    const QColor fgDim      (0x9A, 0x9A, 0x9A);
    const QColor accent     (0x4F, 0x9F, 0xE6);
    const QColor accentText (0xFF, 0xFF, 0xFF);
    const QColor border     (0x4A, 0x4A, 0x4A);

    QPalette pal = root->palette();
    pal.setColor(QPalette::Window,          bgWindow);
    pal.setColor(QPalette::WindowText,      fgText);
    pal.setColor(QPalette::Base,            bgBase);
    pal.setColor(QPalette::AlternateBase,   bgAlt);
    pal.setColor(QPalette::Text,            fgText);
    pal.setColor(QPalette::Button,          bgAlt);
    pal.setColor(QPalette::ButtonText,      fgText);
    pal.setColor(QPalette::ToolTipBase,     bgWindow);
    pal.setColor(QPalette::ToolTipText,     fgText);
    pal.setColor(QPalette::PlaceholderText, fgDim);
    pal.setColor(QPalette::BrightText,      QColor(0xFF, 0xFF, 0xFF));
    pal.setColor(QPalette::Link,            accent);
    pal.setColor(QPalette::Highlight,       accent);
    pal.setColor(QPalette::HighlightedText, accentText);
    pal.setColor(QPalette::Disabled, QPalette::Text,         fgDim);
    pal.setColor(QPalette::Disabled, QPalette::WindowText,   fgDim);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText,   fgDim);
    pal.setColor(QPalette::Disabled, QPalette::Highlight,    QColor(0x55, 0x55, 0x55));
    root->setPalette(pal);

    // Stylesheet for widgets that ignore palette roles or need explicit
    // border/background rules to read well on a dark surface (QSpinBox,
    // QPlainTextEdit, QTabBar, QGroupBox titles, QPushButton). Targeted at
    // descendants of `root` only — we don't want to colour-bleed onto the
    // host TS3 widgets, which already manage their own theming.
    root->setStyleSheet(QStringLiteral(R"(
        QWidget                 { color: #E6E6E6; }
        QLabel                  { color: #E6E6E6; background: transparent; }
        QGroupBox               { color: #E6E6E6;
                                  border: 1px solid #4A4A4A;
                                  border-radius: 4px;
                                  margin-top: 12px;
                                  padding: 8px 8px 6px 8px; }
        QGroupBox::title        { subcontrol-origin: margin;
                                  subcontrol-position: top left;
                                  left: 8px; padding: 0 4px;
                                  color: #E6E6E6; }
        QCheckBox, QRadioButton { color: #E6E6E6; spacing: 6px; }
        QLineEdit, QSpinBox, QPlainTextEdit, QTextEdit {
                                  background: #1E1E1E;
                                  color: #E6E6E6;
                                  border: 1px solid #4A4A4A;
                                  border-radius: 3px;
                                  padding: 2px 4px; }
        QSpinBox::up-button, QSpinBox::down-button {
                                  background: #2B2B2B;
                                  border: 1px solid #4A4A4A; }
        QPushButton             { background: #3A3A3A; color: #E6E6E6;
                                  border: 1px solid #4A4A4A;
                                  border-radius: 3px; padding: 4px 12px; }
        QPushButton:hover       { background: #4A4A4A; }
        QPushButton:pressed     { background: #2B2B2B; }
        QPushButton:disabled    { color: #777777; }
        QSlider::groove:horizontal { background: #1E1E1E; height: 4px;
                                  border-radius: 2px; }
        QSlider::handle:horizontal { background: #4F9FE6;
                                  width: 14px; margin: -5px 0;
                                  border-radius: 7px; }
        QTabWidget::pane        { border: 1px solid #4A4A4A;
                                  background: #2B2B2B;
                                  top: -1px; }
        QTabBar::tab            { background: #2B2B2B; color: #C0C0C0;
                                  padding: 6px 12px;
                                  border: 1px solid #4A4A4A;
                                  border-bottom: none;
                                  border-top-left-radius: 3px;
                                  border-top-right-radius: 3px; }
        QTabBar::tab:selected   { background: #3A3A3A; color: #FFFFFF; }
        QTabBar::tab:hover      { background: #353535; }
    )"));
}
