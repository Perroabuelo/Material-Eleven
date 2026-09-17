#ifndef _ELEVENMPV_MENU_PLAYAUDIO_H_
#define _ELEVENMPV_MENU_PLAYAUDIO_H_

// Starts playback of `path` and shows the Now Playing screen. Tears down
// whatever track was previously loaded first, if any.
void Menu_PlayAudio(char *path);
// Re-enters the Now Playing screen for the track already loaded in the
// background (used by the nav rail from Folders/Settings). No-op if
// nothing is currently loaded.
void Menu_ShowNowPlaying(void);

// Cross-screen playback state/controls for Folders' docked mini-player
// and the nav rail. These reflect the most recently opened track and
// keep working after navigating away from Now Playing, since playback
// is not tied to that screen's loop being the active one.
const char *Music_GetDisplayTitle(void);
const char *Music_GetDisplayArtist(void);
void Music_TogglePlayPause(void);
void Music_Previous(void);
void Music_Next(void);

#endif
