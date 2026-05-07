#pragma once

class QWidget;

// Reads the Windows "Apps use light/dark theme" setting straight from the
// registry — Qt 5.15 has no portable runtime API for it. Returns false on
// non-Windows builds.
bool sv_systemUsesDarkTheme();

// Apply a dark or light palette + complementary stylesheet to a dialog and
// all of its descendants. No-op for the light branch; we leave Qt's default
// in place so the dialog inherits the host application's look.
void sv_applyThemeToDialog(QWidget* root);
