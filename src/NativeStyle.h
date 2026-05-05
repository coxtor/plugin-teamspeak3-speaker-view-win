#pragma once

class QWidget;

// TS3's host QApplication sets its own Qt style + dark palette globally.
// Widgets we create inherit both, so our windows look flat/dark instead of
// like a normal Windows 10/11 dialog. applyNativeWindowsLook() overrides
// both on a per-widget basis: the native "windowsvista" style (rounded
// corners, accent colors, Win11 mica-ish chrome where supported) plus the
// system palette. Call it on every top-level widget we own.
void applyNativeWindowsLook(QWidget* w);
