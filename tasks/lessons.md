# Lessons

Scope: Windows-specific gotchas will land here as they are discovered.

For the TS3-SDK-specific lessons already captured during the Mac
implementation (SDK layout, two-SDK confusion, VAD gaps, phantom
`NOT_TALKING` events, fade-clock integrity), see
`../plugin-teamspeak3-speaker-view/tasks/lessons.md`. Those apply
identically on Windows.

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
