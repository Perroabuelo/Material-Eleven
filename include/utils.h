#ifndef _ELEVENMPV_UTILS_H_
#define _ELEVENMPV_UTILS_H_

void Utils_SetMax(int *set, int value, int max);
void Utils_SetMin(int *set, int value, int min);
int Utils_ReadControls(void);
// Buttons held down as of the last Utils_ReadControls, not the edge that
// `pressed` carries. A hold is what an escape combo has to be made of: an
// edge cannot be distinguished from a stray tap.
SceUInt32 Utils_HeldButtons(void);
// START apaga la pantalla en cada Utils_ReadControls y se quita de `pressed`,
// y la pulsacion que vuelve a encenderla se descarta. Se deshabilita mientras
// un dialogo del sistema tiene la entrada, donde START significa otra cosa.
void Utils_SetScreenOffEnabled(SceBool enabled);
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
