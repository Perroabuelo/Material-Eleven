## 1. Shared visual foundation

- [ ] 1.1 Add `Manrope` and `IBM Plex Mono` (OFL-licensed) font files under `res/` and load them via `vita2d_load_font_file` alongside `Roboto-Regular.ttf` in `main.c`; verify both fonts load without a negative return and render sample text.
- [ ] 1.2 Produce 9-sliced corner-tile textures for the two radius families used across the mockups (small ~10px radius for chips/rows, large ~18-28px radius for cards), as new PNGs loaded via `Textures_Load` (`source/textures.c` / `include/textures.h`); verify a rounded card and a pill chip render correctly at the mockups' reference sizes on hardware or emulator.
- [ ] 1.3 Add a small shared drawing helper (rounded rect via 9-slice, pill, badge, list row) used by all three screens in place of ad hoc `vita2d_draw_rectangle` calls; verify by rendering one sample screen using only the new helpers.
- [ ] 1.4 Define the shared color/spacing/typography constants from the mockups (background `#120F17`/`#17141F`, accent `#FF9166`, text tiers `#F4EFEA`/`#B0A8C0`/`#756C89`/`#6b6478`, chip and card radii) in one shared location referenced by all three screens.

## 2. Navigation shell (`ui/nav-shell`)

- [ ] 2.1 Implement the persistent nav rail (Now Playing / Folders / Settings entries, active-state highlight) as shared chrome drawn and hit-tested inside each of the three screens' loops; verify selecting a rail entry switches screens correctly from each starting screen.
- [ ] 2.2 Preserve existing physical-button navigation (SELECT opens Settings from Folders, START exits, the cancel button returns to the previous screen) alongside the new rail; verify each shortcut still produces its pre-change outcome.
- [ ] 2.3 Add the on-screen control-legend bar reflecting the physical-button actions available on the current screen; verify its content differs correctly between Folders, Settings, and Now Playing.

## 3. Settings screen (`ui/settings`)

- [ ] 3.1 Build the shared master-detail driver described in `design.md` (category list + per-category detail renderer + input handler) and migrate all five categories (storage/device, sort, metadata, dynamic normalizer, equalizer) onto it, replacing `Menu_DisplayDeviceSettings`, `Menu_DisplaySortSettings`, `Menu_DisplayMetadataSettings`, `Menu_DisplayALCModeSettings`, and `Menu_DisplayAudioSettings`.
- [ ] 3.2 Add the current-value hint per category in the category list (active device, sort order, equalizer preset name); verify each hint matches its corresponding `config` field.
- [ ] 3.3 Verify behavior parity for every setting against pre-change behavior: device/root switch persists and takes effect, sort order re-sorts the file list, metadata toggles persist, ALC mode persists, equalizer preset applies immediately (`sceAudioOutSetEffectType`) and persists, and the volume-limit toggle persists.
- [ ] 3.4 Remove the five now-unused standalone settings loop functions and their superseded full-screen rendering code once parity is verified.

## 4. Folders screen (`ui/folder-browser`)

- [ ] 4.1 Reskin `Menu_DisplayFiles`/`Dirbrowse_DisplayFiles` to the mockup's row/breadcrumb/format-badge layout using the shared drawing helpers; verify folder rows and each recognized audio extension (flac/it/mod/mp3/ogg/opus/s3m/wav/xm) render with a correct format badge, and unrecognized files render without one.
- [ ] 4.2 Preserve directory navigation (enter subfolder, return to parent, root as a boundary) and file-opening behavior (recognized extensions start playback, others do nothing); verify against current `Dirbrowse_Navigate`/`Dirbrowse_OpenFile` behavior.
- [ ] 4.3 Add the in-folder filename filter over the already-loaded `File` list (no new directory reads); verify typing a term narrows visible rows to substring matches within the current folder only, and that subfolder contents are never searched or shown.
- [ ] 4.4 Add the docked mini-player showing the active track's title/artist and transport controls while browsing; verify it reflects live playback state and does not interrupt playback.

## 5. Now Playing screen (`ui/now-playing`)

- [ ] 5.1 Reskin `Menu_PlayAudio`'s layout (cover panel, title/artist, format badge, transport controls, seek bar) to the mockup, preserving the existing metadata fallback chain (title+artist, title-only, or filename) and deriving the format badge from the file extension.
- [ ] 5.2 Preserve transport behavior (play/pause, previous/next, shuffle toggle, repeat toggle) and touch-seek behavior; verify each against current `Music_HandleNext`/`Audio_Seek`/`Audio_Pause` behavior.
- [ ] 5.3 Add the read-only "up next" preview sourced from the existing playlist/selection state, including an explicit empty/short-queue visual state for folders with only one or two tracks; verify no control on this screen opens a reorder/remove/clear queue view.
- [ ] 5.4 Restyle the existing status strip (`StatusBar_Display`) to the new palette and typography without changing what data it displays.

## 6. Cross-screen verification

- [ ] 6.1 Walk all nav-rail transitions between the three screens end-to-end on hardware or emulator and confirm the active-destination indicator is correct at each step.
- [ ] 6.2 Review each screen against the `ui/nav-shell` and `ui/now-playing` specs' negative requirements and confirm no control opens Library, Playlists, or a queue-management view.
- [ ] 6.3 Diff `source/fs.c`, `source/utils.c`, and `include/config.h` against the pre-change tree and confirm they are unchanged; diff `source/audio/audio.c`/`include/audio/audio.h` and confirm the only change is the additive `Audio_HasTrack()` accessor (see design.md), with no changes to decoding/DSP logic — verifying decoding/config behavior was not touched.
