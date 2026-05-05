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

## TS3 menu items require ts3plugin_freeMemory to be exported

The SDK contract for `ts3plugin_initMenus` is: you `malloc` the menu
item array and each item, TS3 keeps the pointer, then later calls
`ts3plugin_freeMemory(ptr)` on the array and each entry to release them.

If `ts3plugin_freeMemory` is **not** exported from the plugin, TS3
silently refuses to pick up the menu — no error, no log entry, the
entries just don't appear under Tools → Plugins. Same contract applies
to `ts3plugin_infoData` allocations.

Always export, even if the plugin does not otherwise allocate for TS3:

```cpp
extern "C" void ts3plugin_freeMemory(void* data) { std::free(data); }
```

Allocations passed back to TS3 must come from the same allocator
(`malloc`/`free`, not `new`/`delete`).

## Validate saved window frame against current screens before restoring

A persisted `rememberFrame` geometry can outlive the monitor it was
saved on — typical scenario: user has a second display attached (e.g.
Parallels guest VM detecting a Retina display), saves an overlay
position at x=1980, then later runs without that display. Qt happily
reports `isVisible() == 1` for a window placed at coordinates that
intersect no physical screen, so diagnostics that only check visibility
miss the problem.

On restore, intersect the saved rect with every `QScreen::availableGeometry()`;
if the overlap is smaller than some threshold (e.g. 80x40 px), discard
the saved frame and fall back to the default position. Also, when
saving an empty/invalid QRect, explicitly `QSettings::remove()` the
frame keys — otherwise the old coordinates linger in settings.ini.

## Don't start a QGraphicsEffect animation before first paint

Creating a `QGraphicsOpacityEffect` on a widget and immediately starting
a `QPropertyAnimation` on it **before the widget has ever been painted**
crashes on Qt 5.15 / Windows inside `QGraphicsEffectSource::draw` on
the first paint event. Repro: a row widget constructed as child of a
`Qt::Tool` parent, effect installed in ctor, animation started in the
same ctor. Crash fires later, on the first voice event that makes the
row visible.

Initialise effect state (opacity, labels, flags) synchronously in the
ctor. Start animations only from subsequent update calls driven by the
event loop, not inside the ctor.

## Window flag set at construction must include WindowStaysOnTopHint

Previous lesson here guessed that `Qt::WindowDoesNotAcceptFocus` was
suppressing visibility. Wrong — that was a red herring. The real reason
the overlay never appeared after reworking the flag strategy: the
constructor dropped `Qt::WindowStaysOnTopHint` from the flag set,
expecting a deferred `SetWindowPos(HWND_TOPMOST)` from `showEvent` to
compensate. On Qt 5.15 inside TS3, that deferred native apply does not
reliably make a `Qt::Tool` window visible when `WindowStaysOnTopHint`
wasn't part of the initial flag set.

Ship v0.2's proven flag set at construction:
`Qt::Tool | Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint`
(the WindowStaysOnTopHint conditional on the saved always-on-top pref).
Runtime toggles of always-on-top still go through `SetWindowPos` on the
live HWND to avoid the HWND-rebuild crash — but the *initial* state
must be expressed via Qt flags, not backfilled via Win32 after show.

## Do not touch the Qt style/palette from inside a plugin

We sit inside TS3's QApplication. Any attempt to override its QStyle,
QPalette, or font — even per-widget at construction time — was
unreliable on Qt 5.15 and repeatedly caused the top-level window to
not appear at all. Symptoms ranged from "widget is 0×0 and invisible"
to "entire window client area renders black" depending on which knob
we touched. The only safe strategy is: do nothing. Accept that our
UI inherits the host's theme. The plugin is a guest in TS3's process,
not a standalone app.

## setWindowFlags() recreates the native HWND — avoid runtime changes

On Windows, `QWidget::setWindowFlags()` on an already-realized window
destroys and re-creates the native HWND. That interacts badly with
child widgets carrying `QGraphicsOpacityEffect`s and running
`QPropertyAnimation`s — causing TS3 to crash when the user toggles
always-on-top at runtime.

Set the Qt window flags once, in the constructor, and never call
`setWindowFlags` again. For runtime window-behavior toggles use the
native Win32 APIs directly on the HWND obtained from `winId()`:
- `SetWindowPos(hwnd, HWND_TOPMOST|HWND_NOTOPMOST, ...)` for always-on-top
- `SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex | WS_EX_TRANSPARENT)` for
  click-through
Neither recreates the HWND, so Qt state (children, effects,
animations) stays intact.

## Stop flicking: re-plan instead of patching the same area

When a single area produces multiple regressions in a row (here: first
the window disappeared when toggling borderless, then the same symptom
returned after native-look changes, then the always-on-top toggle
crashed TS3), that is a signal to step back and reconsider the whole
approach rather than adding another guarded condition on top of the
previous fix. Each flick here added a comment about "this one weird
Qt pitfall" while leaving the underlying strategy (mutating window
flags / palette / style at runtime) intact. The fix was structural:
freeze window flags after construction and route runtime changes
through Win32. Compounding flicks hides the root cause.

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
