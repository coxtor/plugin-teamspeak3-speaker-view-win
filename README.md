# plugin-teamspeak3-speaker-view-win

Windows port of [plugin-teamspeak3-speaker-view](../plugin-teamspeak3-speaker-view)
— a TeamSpeak 3 client plugin that shows a separate overlay window listing
members of the current channel who are currently speaking, similar to Mumble's
overlay.

**Status: scaffolding only.** No implementation yet. See "Open decisions"
below — these must be resolved before code is written.

The existing macOS repository is feature-complete (fade-out, config dialog,
frame persistence, VAD-gap handling) and serves as the **reference
implementation**. Any behaviour implemented here must match it.

## Target

- TeamSpeak 3 Client 3.6.x on Windows (x64)
- Plugin API version 26
- Output: `speakerview.dll` loadable from
  `%APPDATA%\TS3Client\plugins\`

## Feature parity (reference: macOS plugin)

| Feature                                | macOS (shipped) | Windows |
|----------------------------------------|-----------------|---------|
| Separate overlay window                | Yes             | Todo    |
| Own-channel filter                     | Yes             | Todo    |
| Fade-out after configurable delay      | Yes             | Todo    |
| 2s VAD grace period (no speech flicker)| Yes             | Todo    |
| Display mode: all / latest only        | Yes             | Todo    |
| Always-on-top toggle                   | Yes             | Todo    |
| Click-through toggle                   | Yes             | Todo    |
| Borderless toggle                      | Yes             | Todo    |
| Remember position/size across restarts | Yes             | Todo    |
| Avatar / channel name per row          | Optional / Yes  | Todo    |
| Show own voice toggle                  | Yes             | Todo    |
| Config via TS3 plugin settings dialog  | Yes             | Todo    |

## Open decisions (need answers before coding)

### 1. Build infrastructure — **blocking**

Neither the user nor the AI assistant has a Windows build machine available.
Options:

- **A. GitHub Actions CI** with a `windows-latest` runner — the
  recommended path. Push code → CI invokes `msbuild` or `cmake` with MSVC
  → uploads a `speakerview.dll` artifact → user downloads it from the
  Actions run → drops it into the TS3 plugin folder. Free for public
  repositories.
- **B. Manual build on a borrowed/virtual Windows machine.** Works but
  every iteration requires manual ceremony.
- **C. Cross-compile from Linux using `clang-cl` + MSVC headers or
  MinGW.** Fragile; difficult to match the MSVC ABI TS3 was built with.

Proposed: **A**, with MSVC via `msbuild` on the runner.

### 2. UI framework — **blocking**

TS3 itself embeds Qt 5.15 (TS3 is a Qt application). That means a Qt-based
plugin uses the Qt already loaded into the TS3 process — **zero runtime
dependencies for the end user.** Options:

- **A. Qt 5.15 (QWidget)** — single codebase that would run on Mac,
  Windows, and Linux. The macOS plugin could later be unified onto this
  path if ever needed. Build-time dependency: a local Qt 5.15.x install
  on the build machine (here, the CI runner).
- **B. Pure Win32 API** — no Qt dependency at build time; everything
  from scratch using `CreateWindowEx`, custom window proc, GDI/Direct2D
  for the rounded-corner HUD look. Lots of boilerplate, but zero external
  dependencies. Does not compose with the Mac codebase.
- **C. WinUI 3 / WPF** — modern Windows-only UI stacks. WinUI 3 requires
  the Windows App SDK runtime installed on every user machine; WPF
  requires .NET. Both are poor fits for a TS3 plugin (which is a native
  DLL loaded into a C++ process).

Proposed: **A (Qt)**, because:
- Feature parity with Mac is the explicit goal.
- TS3 already hosts Qt, so the user gets a zero-install plugin.
- Maps cleanly to TS3's `PLUGIN_OFFERS_CONFIGURE_QT_THREAD` config mode.

### 3. Architecture: shared core vs. fresh rewrite

If Qt is chosen (decision 2A), the natural move is:

- Port `SpeakerTracker` and `ConfigModel` from Objective-C++ to pure C++
  / Qt. These are mostly value logic with no platform-specific UI.
- Rewrite `OverlayController` / `OverlayWindow` / `SpeakerRowView` in
  `QWidget`.
- Rewrite `ConfigWindowController` in `QDialog`.

If pure Win32 is chosen (decision 2B), the tracker/config can still be
shared as plain C++, but the UI is rewritten natively.

### 4. Plugin installation path and settings file location

- Install: `%APPDATA%\TS3Client\plugins\speakerview.dll` (+
  `speakerview.ini`).
- Settings: `%APPDATA%\TS3Client\plugins\speaker-view\settings.ini` (Qt
  `QSettings` IniFormat) OR `settings.json`. Proposed: **INI** to mirror
  the Mac plist.

## What *will not* change from the Mac implementation

These are behavioural invariants established by the Mac reference and
lessons learnt during testing; they must be preserved in the Windows port.

- **VAD grace period: 2 s** before fade-out begins. Avoids flicker during
  natural speech pauses.
- **Phantom-entry guard**: `NOT_TALKING` for a client no longer tracked
  must be a no-op. Do not resurrect entries.
- **Fade-clock integrity**: only the first `talking → not-talking`
  transition stamps the stop time. Subsequent `NOT_TALKING`s during fade
  are no-ops.
- **Config-driven UI**: all options live in the TS3 plugin settings
  dialog and persist to a settings file owned by this plugin (not the
  TS3 registry/ini).
- **Own-channel filter**: only clients in the same channel as the local
  user are shown.
- **Auto-fit height suppressed when `rememberFrame` is on**: the UI
  must not fight the user's manual sizing on every snapshot.
- **Callbacks are off-thread**: TS3 event callbacks are not guaranteed
  to run on the UI thread. Mutations must be serialised and UI updates
  marshalled to the UI thread.

## Reference

Mac implementation lives at `../plugin-teamspeak3-speaker-view`. See
`tasks/lessons.md` there for the TS3-specific gotchas discovered during
live testing — they apply identically to Windows.

## License

MIT — see [LICENSE](LICENSE). TS3 SDK headers belong to TeamSpeak Systems
GmbH and are not redistributed.
