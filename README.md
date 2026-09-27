# Material-Eleven

A homebrew music player for Playstation VITA that aims to support many different audio formats compared to the offical PS VITA music application.

Material-Eleven is a modified fork of [ElevenMPV](https://github.com/joel16/ElevenMPV) by Joel16. On top of the original player it adds:
- A new interface in the style of Material You, with an accent colour taken from the cover art.
- A music library, and a playback queue with independent shuffle and repeat.
- Vector-drawn controls and new font rendering (Manrope, IBM Plex Mono).
- Audio improvements ported from [ElevenMPV-A](https://github.com/GrapheneCt/ElevenMPV-A) by GrapheneCt: following the system volume, hardware EQ presets with an optional limiter, and a configurable output buffer.


# Currently supported formats: (16 bit signed samples)
- FLAC
- IT
- MOD
- MP3 
- OGG
- OPUS
- S3M
- WAV (A-law and u-law, Microsoft ADPCM, IMA ADPCM)
- XM


# Features:
- Browse ux0:/, ur0:/ and uma0:/ to play the above audio formats.
- Pause/Play audio.
- Shuffle/Repeat audio.
- Next/Previous track in current working directory.
- Display ID3v1 and ID3v2 metadata for MP3 files. Other tags are displayed for OGG, FLAC, OPUS and XM.
- Basic touch support.
- Seeking support using touch screen. (No support for OPUS)


# Controls:
**In file manager:**

- Enter button (cross/circle): enter folder/play supported audio file.
- Cancel button (cross/circle): go up parent folder.
- DPAD Up/Down: Navigate files.
- DPAD Left/Right: Top/Bottom of list.

**In audio player: (Note: you can use touch controls here or the following buttons below)**

- Enter button (cross/circle): Play/Pause.
- Cancel button (cross/circle): Return to file manager.
- L trigger: Previous audio file in current directory.
- R trigger: Next audio file in current directory.
- Triangle: Shuffle audio files in current directory.
- Square: Repeat audio files in current directory.
- Start: Turn off display and keep playing audio in background.
- Touch: Touch anywhere on the progress bar to seek to that location.


# License:
Material-Eleven is distributed under the GNU General Public License, version 3 or (at your option) any later version. See [LICENSE](LICENSE).

It is based on ElevenMPV, whose original code is © 2019 Joel16 under the [Apache License 2.0](LICENSES/Apache-2.0.txt), and it includes code derived from ElevenMPV-A, © 2020-2022 GrapheneCt and contributors, under the GPL-3.0-or-later. [NOTICE](NOTICE) records which part comes from where, and lists every third-party component and its license. The license texts are in [LICENSES](LICENSES), and the .vpk carries them, together with LICENSE and NOTICE, in its `licenses/` directory.


# Credits:
- Joel16, for ElevenMPV, the player this project is built on.
- GrapheneCt and the ElevenMPV-A contributors, for the audio pipeline improvements ported from ElevenMPV-A.
- MPG123 contributors.
- dr_libs by mackron.
- libFLAC, libvorbis, libogg, libopus and opusfile contributors (Xiph.Org).
- libxmp-lite contributors.
- vita2d by xerpi, and the FreeType, libpng, libjpeg-turbo, zlib and bzip2 projects. This software is based in part on the work of the Independent JPEG Group.
- Preetisketch for startup.png (banner).
- LineageOS's Eleven Music Player contributors for design elements.
- Manrope and IBM Plex Mono authors, fonts under the SIL Open Font License 1.1.
