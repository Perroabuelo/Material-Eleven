# Changelog

All notable changes to the app are recorded in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/). Version `X.Y.Z` shows on the console as `XX.YZ`
(for example, 3.0.0 is `03.00`).

## [Unreleased]

### Added

- Developer test suites: with the developer log on, a `suite.txt` in `ux0:data/ElevenMPV/` makes the app replay scripted button presses and touches, check the result of each scenario and record it in `debug.log`, then close by itself. Pressing any button stops the suite.

## [4.2.0] - 2026-10-17

### Added

- Developer log, off by default: create an empty file named `debug` in `ux0:data/ElevenMPV/` and the app records screens, input and playback events to `ux0:data/ElevenMPV/debug.log`. The previous session is kept as `debug.prev.log`.
- The debug overlay shows whether the log is on, how many lines it has written and how many it dropped.

## [4.1.0] - 2026-10-16

### Added

- Select several songs at once: choose "Select" from the Square menu, or touch and hold a row.
- Add all selected songs, albums or artists to a playlist in one go.
- Remove several songs from a playlist at once.
- The "All" checkbox at the top selects every row in view, so you can search and then add all the results. Search again to keep adding to the same selection.

## [4.0.0] - 2026-10-14

### Added

- New Playlists tab in the Library: make your own playlists right on the Vita.
- Press Square on a song, an album or an artist and choose "Add to playlist" to add it to an existing playlist or to a new one.
- Inside a playlist, press Square on a song to remove it or move it to another position.
- Rename or delete a playlist with Square in the Playlists tab.
- Each playlist shows a cover made from the first four albums in it.
- Songs whose file is gone are shown dimmed and skipped during playback.

## [3.9.0] - 2026-10-12

### Added

- Library, Folders and Settings lists now scroll with your finger, and keep gliding after a flick.
- Tap a row to play it or open it; tap the Library tabs, empty-screen actions and Settings entries too.

### Changed

- Scrolling with touch never plays anything by accident: only a tap does.

## [3.8.0] - 2026-10-11

### Added

- Search your library: press Triangle or tap the search box to filter the current list by title, artist or album.
- Triangle now opens the filter in Folders too.

### Changed

- Playing a song from search results keeps the whole list as the queue, starting at that song.
- Square in the Library now opens a menu with Rescan and Choose folder.
- Search ignores case and accents: "cancion" finds "Canción", in Folders and Library.

### Fixed

- Fixed accented letters typed into the folder filter being garbled.

## [3.7.0] - 2026-10-11

### Added

- Holding Up or Down in a list now keeps moving the selection, so you no longer have to press once per row.
- Keep holding and the list starts jumping a page at a time, to get through long libraries quickly.
- Holding stops at the top or bottom of the list; a new press still wraps around.

## [3.6.0] - 2026-10-11

### Added

- Drag the progress bar to choose where to jump. The bar grows and shows a handle while you hold it, the elapsed time follows your finger, and the song jumps there when you let go.

### Fixed

- Touching the end of the progress bar now goes to the end of the song. Before, it stopped at about 95 %, and the filled part of the bar did not line up with your finger.
- The progress bar is easier to hit: its touch area is taller.

## [3.5.1] - 2026-10-11

### Fixed

- When a track ended while you were on Folders, Library or Settings, the next one did not start. The queue now moves on, or repeats the track, from any screen, including with the screen turned off.

## [3.5.0] - 2026-10-08

### Added

- Now Playing can show the song's lyrics: press Up on the D-pad to open a full-screen lyrics view, and again to close it.
- Synced lyrics follow the song line by line, with the current line highlighted.
- Drag the lyrics to look around; after a few seconds the view returns to the line being sung.
- Lyrics are read from a `.lrc` file next to the track (same name) or from the lyrics embedded in the file (FLAC, OGG and Opus `LYRICS`, MP3 `USLT`).
- Lyrics without timestamps are shown as scrollable text, and tracks without lyrics say so.
- `.lrc` files should be saved as UTF-8; files in other encodings such as Shift-JIS or GBK may show wrong characters.

## [3.4.3] - 2026-10-07

### Fixed

- Fixed colored streaks and garbled text that sometimes flashed across the screen right after changing tracks.

## [3.4.2] - 2026-10-04

### Fixed

- Fixed memory leaks when changing tracks, when playing tracker modules (MOD, XM, IT, S3M) and when a file can't be opened.
- Fixed file names containing "%" showing garbled on Now Playing.
- Fixed a crash when playing mono MP3 files, and a possible memory overrun on some Opus files.
- A file named .mp3 that isn't really an MP3 now shows the "can't play" notice in the Library instead of opening a silent track.

## [3.4.1] - 2026-10-03

### Fixed

- Fixed a crash that could happen after switching between screens many times in a long session.

## [3.4.0] - 2026-10-03

### Added

- New setting under Settings > Playback: when an album or artist ends, repeat it or keep playing the next one.
- Artists in the Library now show a cover from one of their albums.
- Up Next on Now Playing shows the cover of each upcoming song when you play from the Library.

### Changed

- Albums in the Library now play in disc order, by disc and track number, instead of alphabetically by title.
- Inside an artist, songs are grouped by album and follow each album's track order.
- Your existing library is kept after updating. Rescan with Triangle to read track numbers; you can stop and resume at any time.

## [3.3.1] - 2026-10-03

### Fixed

- The back button on Now Playing now returns you to the screen where you picked the song. From the Library, you land on the same tab, album or artist, and row you left.

## [3.3.0] - 2026-10-03

### Added

- Black and white covers now give the interface a light neutral accent, instead of the orange used when a song has no cover.

### Fixed

- Covers with a small detail of color, such as a red title over a gray background, now tint the interface with that color.

## [3.2.0] - 2026-10-02

### Added

- The bottom bar shows each button's icon in its own color, and follows whether your console confirms with Circle or Cross.
- Format badges now also appear on songs in the Library.

### Changed

- START turns the screen off from any screen while the music keeps playing. To close the app, use the PS button.
- With the screen off, the PS button turns it back on.

### Fixed

- Format badges now show the extension (FLAC, MP3, OGG...) centered and readable.
- The console no longer suspends on its own while music plays with the screen off.

## [3.1.1] - 2026-09-28

### Changed

- New icon, LiveArea and boot screen, in the app's style.
- The app shows on the console as "Material Eleven".
- Installing over the previous version keeps your library and settings.

## [3.1.0] - 2026-09-27

### Added

- Every version publishes its `.vpk` on the GitHub Releases page.
- The app is now available in English and Spanish.
- On first launch it uses your console's language: Spanish if the console is set to Spanish,
  English otherwise.
- New **Language** option in Settings to choose between System, English and Español, applied
  instantly.

### Changed

- The Spanish texts were reviewed: full spelling and neutral Spanish.
- Updating to this version keeps all your settings.

## [3.0.0] - 2026-09-27

First version of Material-Eleven, the fork of Joel16's ElevenMPV 2.10.

### Added

- New Material You style interface, with an accent color taken from the cover art.
- Music library and a playback queue, with independent shuffle and repeat.
- Vector-drawn controls and new typefaces (Manrope, IBM Plex Mono).
- Volume follows the system volume control.
- Audio settings with hardware EQ presets and an optional limiter, brought over from ElevenMPV-A.

### Changed

- The project has its own repository, Perroabuelo/Material-Eleven.
- The project is distributed under GPL-3.0-or-later, with full credit to ElevenMPV (Joel16),
  ElevenMPV-A (GrapheneCt) and the libraries it uses (see NOTICE).
- The `.vpk` includes, in `licenses/`, the project license, the NOTICE and the licenses of
  every library and typeface it ships.
