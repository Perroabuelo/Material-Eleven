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
	// la que la vuelve a encender. Si el usuario la enciende con el boton PS,
	// que la aplicacion no ve, sigue armado y descarta la pulsacion siguiente.
	int armed;
} ScreenOff_State;

void ScreenOff_Init(ScreenOff_State *s, unsigned int start, unsigned int ltrigger, unsigned int rtrigger);

// Lo que hay que hacerle a la pantalla despues de un fotograma.
typedef enum {
	SCREEN_OFF_KEEP = 0,
	SCREEN_OFF_TURN_OFF,
	SCREEN_OFF_TURN_ON
} ScreenOff_Action;

// Recibe el flanco y los botones mantenidos de este fotograma y devuelve el
// flanco que deben ver las pantallas. Deja en *action lo que hay que hacerle a
// la pantalla.
//
// - Deshabilitado, el flanco pasa intacto.
// - Armado, el primer flanco pide encender la pantalla, se descarta entero y
//   desarma el filtro. Con la pantalla apagada por la aplicacion la consola
//   solo la enciende con el boton PS, pero los demas botones siguen llegando:
//   sin esto, una pulsacion a ciegas actuaria sobre una pantalla que no se ve.
// - START sin L y R mantenidos a la vez pide apagar, arma el filtro y se quita
//   del flanco. L + R + START pasa intacto.
unsigned int ScreenOff_Filter(ScreenOff_State *s, unsigned int pressed, unsigned int held, ScreenOff_Action *action);

#endif
