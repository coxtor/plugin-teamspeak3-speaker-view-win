# plugin-teamspeak3-speaker-view-win — Implementation Checklist

## Phase 0: Decisions (blocking)

- [ ] **Confirm build-infra plan**: GitHub Actions with `windows-latest`
      runner, MSVC. Alternatives are not viable given no local Windows
      machine.
- [ ] **Confirm UI framework**: Qt 5.15 (shared codebase path) vs. pure
      Win32 (minimal deps path). Default proposal: Qt 5.15.
- [ ] **Confirm settings persistence format**: INI via `QSettings` (if
      Qt) or hand-written INI parser (if Win32). Default: INI.
- [ ] **Decide**: should the Mac plugin be re-unified onto the Qt
      codebase, or does the Mac repo stay as-is in parallel? Default:
      Mac repo stays as-is; this repo is Windows-first, but the code
      should be portable so reunification remains an option.

## Phase 1: Scaffolding

- [x] Repo initialised, `.gitignore`, `LICENSE`, `README.md` with
      decision section, `sdk/README.md`, `tasks/` skeleton.
- [ ] `.github/workflows/build.yml` — CI matrix: Release x64 with MSVC,
      produces `speakerview.dll` as artifact.
- [ ] Build system file (`CMakeLists.txt` or `speakerview.vcxproj`) —
      choice depends on UI framework decision.
- [ ] `resources/speakerview.ini` — TS3 plugin metadata (Platforms=win).

## Phase 2: Core (framework-independent)

- [ ] `src/plugin.cpp` — TS3 C-ABI entry points. Port of
      `../plugin-teamspeak3-speaker-view/src/plugin.mm`.
- [ ] `src/PluginContext.{h,cpp}` — singleton wiring, cross-platform.
- [ ] `src/TS3Bridge.{h,cpp}` — typed wrappers around `ts3_functions`
      with `freeMemory` RAII.
- [ ] `src/SpeakerTracker.{h,cpp}` — ported from `SpeakerTracker.mm`,
      replacing GCD with `std::thread` + `std::mutex` + `std::condition_variable`
      (or `QThread` if Qt). Must preserve: 2s grace, phantom-entry
      guard, fade-clock integrity.
- [ ] `src/ConfigModel.{h,cpp}` — replacing plist with INI; same key
      names and defaults as Mac.

## Phase 3: UI

(Depends on UI framework decision.)

**If Qt:**
- [ ] `src/OverlayWindow.{h,cpp}` — `QWidget`/`QDialog` with `FramelessWindowHint`
      + translucent background for the HUD look.
- [ ] `src/SpeakerRowWidget.{h,cpp}` — one row widget (dot + nick +
      channel).
- [ ] `src/OverlayController.{h,cpp}` — consumes snapshots, reparents
      rows, runs opacity animations (`QPropertyAnimation`).
- [ ] `src/ConfigDialog.{h,cpp}` — `QDialog` with slider, checkboxes,
      radio buttons. Opens via `ts3plugin_configure` in `QT_THREAD` mode.

**If Win32:**
- [ ] Custom `WNDCLASS` + window proc for the overlay.
- [ ] Direct2D drawing for rounded corners + translucency.
- [ ] Standard-controls config dialog (CreateDialog with a .rc template).

## Phase 4: Verification (via CI + user smoke test)

- [ ] CI produces `speakerview.dll` and uploads artifact.
- [ ] User downloads DLL, places into
      `%APPDATA%\TS3Client\plugins\`, restarts TS3.
- [ ] Plugin appears in plugin list; "Settings" opens config dialog.
- [ ] Speaker flow matches Mac behaviour (VAD grace, fade, display
      modes, channel switching).
- [ ] Frame persistence works across TS3 restarts.

## Review

(To be filled in once implementation happens.)
