#ifndef _ELEVENMPV_UTILS_H_
#define _ELEVENMPV_UTILS_H_

#include "key_repeat.h"

void Utils_SetMax(int *set, int value, int max);
void Utils_SetMin(int *set, int value, int min);
int Utils_ReadControls(void);
// Buttons held down as of the last Utils_ReadControls, not the edge that
// `pressed` carries. A hold is what an escape combo has to be made of: an
// edge cannot be distinguished from a stray tap.
SceUInt32 Utils_HeldButtons(void);
// Moves a list selection with this frame's Up/Down: a new press steps one row
// and wraps around; holding repeats the step, one row and later `page` rows at
// a time, and stops at either end. Only lists call it; `pressed` never repeats.
void Utils_ListStep(int *sel, int count, int page);
// This frame's Up/Down step itself, for a list that moves by its own rule
// (the Library's selection mode, which has the "All" checkbox in the loop).
KeyRepeat_Step Utils_GetListStep(void);
// START apaga la pantalla en cada Utils_ReadControls y se quita de `pressed`;
// la consola la vuelve a encender con el boton PS. Se deshabilita mientras un
// dialogo del sistema tiene la entrada, donde START significa otra cosa.
void Utils_SetScreenOffEnabled(SceBool enabled);
// Once at start and once at shutdown, on the UI thread: the power callback
// that tells when the console turns the screen back on.
void Utils_InitPowerCallback(void);
void Utils_TermPowerCallback(void);
// True from the moment START turns the screen off until the console turns it
// back on. Always false if the power callback could not be registered.
SceBool Utils_DisplayOff(void);
int Utils_InitAppUtil(void);
// Una vez al arrancar, después de Utils_InitAppUtil: pasa al subsistema de
// diálogos comunes el idioma y la asignación de botones de la consola.
void Utils_InitCommonDialog(void);
int Utils_TermAppUtil(void);
int Utils_GetEnterButton(void);
int Utils_GetCancelButton(void);
int Utils_Alphasort(const void *p1, const void *p2);
char *Utils_Basename(const char *filename);
void Utils_InitPowerTick(void);

#endif
