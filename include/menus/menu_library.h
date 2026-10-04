#ifndef _ELEVENMPV_MENU_LIBRARY_H_
#define _ELEVENMPV_MENU_LIBRARY_H_

#include "ui_screen.h"

// La cuarta pantalla: la coleccion indexada, y las afordancias de elegir
// carpeta y reescanear. No reemplaza al navegador de carpetas; son dos caminos
// en paralelo hacia la reproduccion. Corre hasta que el usuario elige otra
// pantalla, y la devuelve.
UI_Screen Menu_DisplayLibrary(void);

#endif
