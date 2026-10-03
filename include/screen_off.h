#ifndef _ELEVENMPV_SCREEN_OFF_H_
#define _ELEVENMPV_SCREEN_OFF_H_

// Que hace START con la pantalla, y que pulsacion se descarta al encenderla.
//
// Es logica pura: no incluye vita2d ni headers de SCE, para poder probarla en PC
// (tests/test_screen_off.c). Los botones llegan como mascaras, y quien apaga la
// pantalla de verdad es Utils_ReadControls (source/utils.c).

typedef struct {
	// Mascaras de START, L y R en el pad de la consola.
	unsigned int start, ltrigger, rtrigger;
	// Con 0 el filtro deja pasar todo sin tocarlo. Se apaga mientras un dialogo
	// del sistema tiene la entrada, donde L + R + START significa otra cosa.
	int enabled;
	// 1 desde que START apago la pantalla hasta la siguiente pulsacion, que es
	// la que la vuelve a encender.
	int armed;
} ScreenOff_State;

void ScreenOff_Init(ScreenOff_State *s, unsigned int start, unsigned int ltrigger, unsigned int rtrigger);

// Recibe el flanco y los botones mantenidos de este fotograma y devuelve el
// flanco que deben ver las pantallas. Pone *request_off a 1 cuando hay que
// apagar la pantalla, y a 0 en cualquier otro caso.
//
// - Deshabilitado, el flanco pasa intacto.
// - Armado, el primer flanco se descarta entero y el filtro se desarma.
// - START sin L y R mantenidos a la vez pide apagar, arma el filtro y se quita
//   del flanco. L + R + START pasa intacto.
unsigned int ScreenOff_Filter(ScreenOff_State *s, unsigned int pressed, unsigned int held, int *request_off);

#endif
