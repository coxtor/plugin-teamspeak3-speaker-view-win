# plugin-teamspeak3-speaker-view-win — Implementation Checklist

## Decisions (locked)

- UI: Qt 5.15 (WidgetKit)
- Compiler: MSVC x64
- Build: CMake
- CI: GitHub Actions `windows-latest`
- Settings: Qt `QSettings` IniFormat at
  `%APPDATA%/TS3Client/plugins/speaker-view/settings.ini`
- Config dialog mode: `PLUGIN_OFFERS_CONFIGURE_QT_THREAD`

## Scaffolding

- [x] `.gitignore`, `LICENSE`, `README.md`
- [x] `sdk/README.md`
- [x] `tasks/todo.md`, `tasks/lessons.md`
- [ ] `CMakeLists.txt`
- [ ] `.github/workflows/build.yml` (real workflow)
- [ ] `resources/speakerview.ini` (plugin metadata)

## Core (cross-platform C++)

- [ ] `src/plugin.cpp` — TS3 C-ABI exports, QT_THREAD configure
- [ ] `src/PluginContext.{h,cpp}` — singleton wiring
- [ ] `src/TS3Bridge.{h,cpp}` — `ts3_functions` wrappers with
      `freeMemory` RAII, returning QString
- [ ] `src/SpeakerTracker.{h,cpp}` — QMutex-protected state, QTimer
      fade tick, emits snapshot via Qt signal. Preserve the three
      invariants from Mac (see README).
- [ ] `src/ConfigModel.{h,cpp}` — wraps QSettings, emits changed
      signal

## UI (Qt)

- [ ] `src/OverlayWidget.{h,cpp}` — top-level Qt::Tool QWidget,
      WindowStaysOnTopHint (configurable),
      WA_TransparentForMouseEvents (click-through)
- [ ] `src/SpeakerRowWidget.{h,cpp}` — dot + nick (+ channel)
- [ ] `src/OverlayController.{h,cpp}` — snapshot consumer, animates
      opacity with QPropertyAnimation
- [ ] `src/ConfigDialog.{h,cpp}` — slider, checkboxes, radio buttons

## Verification (via CI + user smoke test)

- [ ] CI green: `speakerview.dll` artifact produced
- [ ] User downloads artifact → drops into plugin dir → reloads
- [ ] Plugin appears, config dialog opens
- [ ] Sprecher-Flow: other user speaks → name appears → fades
- [ ] Display-mode switch, channel switch, always-on-top,
      click-through, remember-frame toggles observable

## Review

_To be filled in once a first CI green + user smoke test happens._
