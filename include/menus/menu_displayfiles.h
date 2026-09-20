#ifndef _ELEVENMPV_MENU_DISPLAYFILES_H_
#define _ELEVENMPV_MENU_DISPLAYFILES_H_

#include <psp2/types.h>

void Menu_DisplayFiles(void);

// Abre esta misma pantalla en modo seleccion de carpeta: la misma
// navegacion y los mismos controles que el usuario ya conoce, pero
// confirmar elige la carpeta actual en vez de reproducir, y nada suena.
// SCE_TRUE si eligio, con la ruta en `out`. Donde estaba parado el
// navegador se restituye en los dos casos.
SceBool Menu_PickFolder(char *out, int cap);

#endif
