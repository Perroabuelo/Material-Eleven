#ifndef _ELEVENMPV_MINI_PLAYER_H_
#define _ELEVENMPV_MINI_PLAYER_H_

#include <psp2/types.h>

// El mini reproductor acoplado: titulo, artista y transporte de la pista en
// curso, sin interrumpirla.
//
// Vivia privado en la pantalla de carpetas, que era la unica que lo tenia. La
// biblioteca es tambien una pantalla de lista desde la que se reproduce, asi
// que lo comparte en vez de llevar un segundo que habria que mantener alineado
// con este.

// Alto de la banda. Quien lo dibuje tiene que descontarlo de su contenido.
#define MINI_PLAYER_H 72

// Y de su borde superior.
float MiniPlayer_Top(void);

// No dibuja nada si no hay ninguna pista cargada.
void MiniPlayer_Draw(void);

// SCE_TRUE si el toque de este fotograma cayo en uno de sus controles, para que
// la pantalla que lo aloja no lo interprete ademas como suyo.
SceBool MiniPlayer_HandleTouch(void);

#endif
