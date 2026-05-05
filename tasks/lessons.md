# Lessons

Scope: Windows-specific gotchas will land here as they are discovered.

For the TS3-SDK-specific lessons already captured during the Mac
implementation (SDK layout, two-SDK confusion, VAD gaps, phantom
`NOT_TALKING` events, fade-clock integrity), see
`../plugin-teamspeak3-speaker-view/tasks/lessons.md`. Those apply
identically on Windows.

## Never use deleteLater() / deferred work in ts3plugin_shutdown

TS3 unloads the plugin DLL **immediately** after `ts3plugin_shutdown`
returns. Any queued events targeting code that lives in the DLL —
`QObject::deleteLater()`, `QTimer::singleShot(...)` with a DLL-local
lambda, `Qt::QueuedConnection` signals — will later be dispatched into
freed memory and crash TS3. Reproduce: Tools → Options → Addons →
Plugins → "Reload All".

Shutdown must be fully synchronous:
- `delete m_widget;` (not `deleteLater()`)
- Before delete, widget must `hide()` and flush any pending state
- Rows/children get cleaned up transitively by parent deletion

During normal runtime `deleteLater()` is still fine — the DLL is alive
and the event loop can dispatch.

## TS3 scans every .dll in plugins/ for ts3plugin_* exports

Do not ship companion DLLs (VC runtime, Qt redistributables, helper libs)
in `%APPDATA%\TS3Client\plugins\` — TS3 tries to load each one as a
plugin and spams "required plugin function name()/version()/... not
found" errors on every launch (once per rogue DLL).

Runtime dependencies must go:
- into a subfolder the plugin loads manually via `LoadLibrary`, OR
- installed system-wide (preferred for VC runtime — require users to
  install `vc_redist.x64.exe`), OR
- next to `ts3client_win64.exe` itself (only works if the user also has
  write access there, which they usually don't).

## MSVC without /utf-8 mangles non-ASCII string literals

MSVC defaults to reading source files as the active Windows code page
(cp1252 on most locales). Em-dashes, en-dashes, umlauts etc. embedded
as UTF-8 in .cpp files get reinterpreted as cp1252 at compile time and
stored as mojibake in the binary, then displayed as garbage in the UI.

Fix globally in `CMakeLists.txt`: `target_compile_options(... /utf-8)`.
This flag both reads source as UTF-8 and emits UTF-8 string literals,
which matches what Qt expects from `QStringLiteral()`.

## QStyleFactory may return nullptr — don't fall through to QPalette()

On the Qt 5.15 that TS3 ships, not all native style plugins
(`windows11`, `windowsvista`, `windows`) are guaranteed to be
available. If `QStyleFactory::create(...)` returns nullptr for all
candidates and the code path then does `widget->setPalette(QPalette())`
as a fallback, the widget becomes all-black (see earlier lesson on
default-constructed QPalette). Always guard: if no native style is
available, leave the host's style and palette untouched.

## Two Qt-palette pitfalls when toggling display modes

1. `QPalette()` (default-constructed) is not "reset to host" — it is an
   all-black palette. Assigning it blacks out the widget. To revert to
   the host-provided default, call `QApplication::palette()` and assign
   that instead.

2. `setAttribute(Qt::WA_TranslucentBackground, ...)` must be set
   **before** `setWindowFlags(...)`. Changing window flags rebuilds the
   native window; if the attribute isn't set at that point, Qt creates
   the window without `WS_EX_LAYERED` on Windows and DWM will not
   composite the alpha channel. Symptom: toggling into frameless/HUD
   mode makes the window vanish entirely.

## WA_TranslucentBackground needs FramelessWindowHint on Windows

Setting `Qt::WA_TranslucentBackground` unconditionally on a top-level
`QWidget` renders the client area as **opaque black** on Windows unless
the window is also frameless (`Qt::FramelessWindowHint`). DWM only
composites the alpha channel for frameless windows. If the attribute is
set and a custom `paintEvent` then draws a semi-transparent fill on top,
the result is a solid black box that looks like a rendering bug.

Apply `WA_TranslucentBackground` inside `applyWindowFlags()` only when
the user has toggled borderless on. When the flag is off, fall back to
`setAutoFillBackground(true)` and let the native tool-window background
draw.

## TS3 SDK CDN (files.teamspeak-services.com) was retired in 2026

Old URL `files.teamspeak-services.com/releases/sdk/3.6.0/ts3client-pluginsdk-26.zip`
returns 404. Use `github.com/teamspeak/ts3client-pluginsdk` instead,
pinned to a commit SHA. Still API version 26 (confirmed in
`src/plugin.c`).
