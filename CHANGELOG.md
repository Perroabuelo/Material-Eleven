# Changelog

All notable changes to the app are recorded in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/). Version `X.Y.Z` shows on the console as `XX.YZ`
(for example, 3.0.0 is `03.00`).

## [Unreleased]

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
