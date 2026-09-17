## Why

ElevenMPV's UI today is a set of hand-placed, flat immediate-mode screens with no shared visual language: `Menu_PlayAudio`, `Menu_DisplayFiles`/`Dirbrowse_DisplayFiles`, and the five near-identical full-screen list loops in `menu_settings.c` (the repository's lowest-health file per the code index) each draw their own gray bars, white-background rows, and hardcoded pixel layouts independently. The user has produced pixel-precise Material3-inspired mockups (dark palette, rounded cards, pill chips/badges, a persistent nav rail, `Manrope`/`IBM Plex Mono` type) covering every screen the app has today, plus three screens for functionality that doesn't exist yet (Library, Playlists, a mutable Queue). Restyling the existing screens now — before that new functionality is built — gives the shared visual language and component vocabulary a place to land, so the later functionality work extends a real UI system instead of five more copy-pasted loops.

This change is scoped to the visual/navigation layer, with one necessary exception: `source/audio/*` gained a minimal, additive `Audio_HasTrack()` accessor (see Impact) so playback can survive navigating away from Now Playing. Decoding, file I/O, and config semantics are otherwise unchanged.

## What Changes

- Replace the current flat gray/white immediate-mode chrome on every existing screen with the mockups' dark Material3-inspired visual language: background `#120F17`/`#17141F`, accent `#FF9166`, rounded cards and pill chips/badges, `Manrope` body type and `IBM Plex Mono` for numeric/technical labels.
- Introduce a persistent left navigation rail with entries only for destinations that exist today: Now Playing, Folders, Settings. Library and Playlists are **not** added to the rail in this change — an icon that opens nothing would misrepresent what the app can do.
- Reskin Now Playing (today's `Menu_PlayAudio`) to the mockup's layout: rounded cover-art panel, format badge, accent-colored circular transport controls, and a real "up next" mini-preview sourced from the existing in-memory playlist (`menu_audioplayer.c`'s `playlist`/`selection`). The mockup's full-screen Queue view (remove-track, "mezclar cola", "vaciar cola") and its Triangle/Square physical-button remap are **not** included — the underlying mutable-queue model doesn't exist, and per the "hide until it's real" rule this change does not add a button that opens nothing.
- Playback now survives navigating away from Now Playing (via the nav rail or the cancel button) to Folders or Settings, instead of stopping — this is what makes the docked mini-player and free rail navigation between all three screens possible. It only stops when the user opens a different track or exits the app.
- Replace Settings' five near-duplicated full-screen list loops (`Menu_DisplayDeviceSettings`, `Menu_DisplaySortSettings`, `Menu_DisplayMetadataSettings`, `Menu_DisplayALCModeSettings`, `Menu_DisplayAudioSettings` in `menu_settings.c`) with a single master-detail screen (categories on the left, the active category's controls on the right). Every existing option and its behavior (`config.device`, `config.sort`, `config.meta_flac/mp3/opus`, `config.alc_mode`, `config.eq_mode`/`config.eq_volume`) is preserved as-is; only the presentation and navigation shape changes.
- Reskin Folders (today's `Menu_DisplayFiles`/`Dirbrowse`) to the mockup's row/breadcrumb/format-badge layout, and add a simple in-folder filename filter over the already-loaded directory listing (no new I/O, no recursive scan, no index — this is presentation-layer filtering of data the screen already holds).
- Ship replacement typography assets (equivalents of `Manrope`/`IBM Plex Mono`) alongside or in place of the current `res/Roboto-Regular.ttf`, and new/replacement art assets for rounded cards, pills, and nav rail icons.
- **Explicitly out of scope, deferred to a later functionality-focused change**: the Library screen and its scan/index engine, the Playlists screen and all persistence behind it, the mutable Queue screen (remove/shuffle/clear), and the Now Playing physical-button remap tied to that Queue.

## Capabilities

### New Capabilities
- `ui/nav-shell`: the persistent left navigation rail and the top-level screen-switching model across the destinations that exist today (Now Playing, Folders, Settings).
- `ui/now-playing`: the reskinned now-playing/transport screen, including a read-only "up next" preview sourced from the existing playlist.
- `ui/folder-browser`: the reskinned folder-browsing screen (list rows, breadcrumb, format badges, docked mini-player) and its new in-folder filename filter.
- `ui/settings`: the reskinned settings screen as a single master-detail surface, replacing the five separate full-screen menus while preserving every existing option and its behavior.

### Modified Capabilities
_None — this is a greenfield OpenSpec project with no existing specs; nothing is being modified at the requirements level._

## Impact

- **Now Playing**: `source/menus/menu_audioplayer.c`, `include/menus/menu_audioplayer.h`.
- **Folders**: `source/dirbrowse.c`, `include/dirbrowse.h`, `source/menus/menu_displayfiles.c`, `include/menus/menu_displayfiles.h`.
- **Settings**: `source/menus/menu_settings.c`, `include/menus/menu_settings.h` (no changes to `include/config.h` — same fields, same semantics).
- **Navigation shell / entry point**: `source/main.c` (today calls `Menu_DisplayFiles()` directly with no persistent chrome around it).
- **Assets**: `include/textures.h` and its loader (new/replacement PNGs for rounded chrome and nav rail icons), plus new font files alongside `res/Roboto-Regular.ttf`.
- **Playback lifecycle**: `source/audio/audio.c`, `include/audio/audio.h` — added `Audio_HasTrack()` (a session-loaded flag) so other screens and the nav rail can tell whether a track is loaded in the background; no changes to decoding or DSP logic.
- **Not touched**: `source/fs.c`, `source/utils.c`, `include/touch.h` (reused as-is) — no decoding or file-I/O behavior changes.
