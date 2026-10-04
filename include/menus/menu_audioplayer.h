#ifndef _ELEVENMPV_MENU_PLAYAUDIO_H_
#define _ELEVENMPV_MENU_PLAYAUDIO_H_

#include <psp2/types.h>

#include "ui_screen.h"

// Starts playback of `path` and asks to go to Now Playing through
// NavRequest_Set; the screen that called it goes there once it takes the
// request. Tears down whatever track was previously loaded first, if any.
// SCE_FALSE si el archivo no se pudo decodificar; entonces no se pide ir a
// Now Playing y no suena nada.
SceBool Menu_PlayAudio(char *path);
// Igual, pero sin construir la cola: la trae hecha quien llama. Es lo que
// usa la biblioteca, cuya cola es la vista y no la carpeta del archivo.
SceBool Menu_PlayQueued(const char *path);
// Runs the Now Playing screen for the track already loaded until the user
// leaves it, and returns the screen to go to next. Returns UI_SCREEN_FOLDERS
// at once if nothing is loaded.
UI_Screen Menu_ShowNowPlaying(void);

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
