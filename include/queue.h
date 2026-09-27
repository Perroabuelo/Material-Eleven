#ifndef _ELEVENMPV_QUEUE_H_
#define _ELEVENMPV_QUEUE_H_

#include <psp2/types.h>

// La cola de reproduccion: la lista de rutas por la que avanzan siguiente y
// anterior, y por donde va esa lista.
//
// Hasta aqui la cola no existia como cosa: era cwd releido en cada play, y por
// tanto no habia forma de reproducir algo que no fuera "la carpeta actual".
// Teniendola con dueño, quien la llena pasa a ser una decision de quien pulsa
// play - hoy una carpeta, y mas adelante una vista de biblioteca.

// Vacia la cola y devuelve el plan al principio. El barajado sigue como estaba.
void Queue_Clear(void);

// Anade una ruta al final, con el nombre por el que la vista de origen la
// conoce. `title` puede ser NULL - el navegador de carpetas no sabe mas que
// la ruta -, y entonces quien la muestre se queda con el nombre de archivo.
// SCE_FALSE si ya no cabe.
SceBool Queue_Add(const char *path, const char *title);

int Queue_Count(void);

// NULL si el indice cae fuera de la cola.
const char *Queue_GetPath(int index);

// El nombre con el que se metio, o NULL si quien la lleno no traia ninguno.
const char *Queue_GetTitle(int index);

// Puesto dentro del plan, que con el barajado apagado coincide con el indice
// natural. Se retira cuando nadie lea ya vecinos por indice.
int Queue_GetPosition(void);

// Sin acotar: quien llama decide que significa salirse por cada extremo, que
// no es lo mismo al avanzar que al repetir.
void Queue_SetPosition(int index);

// Indice de esa ruta exacta dentro de la cola, o 0 si no esta.
int Queue_IndexOf(const char *path);

// El orden de reproduccion. Por dentro la cola tiene dos espacios de indice -
// el natural, en que puesto entro cada pista, y el slot, en que puesto del plan
// suena -, y por eso por fuera solo habla en rutas: un indice que no dice en cual
// de los dos esta es el que plantaba el puesto equivocado en cada avance.

// Enciende o apaga el barajado sin mover la pista en curso. Al encenderlo queda
// al frente del plan y el resto se baraja detras; al apagarlo el plan vuelve al
// orden natural y lo siguiente es su vecino natural.
void Queue_SetShuffle(SceBool on);

SceBool Queue_IsShuffled(void);

// Mueve un puesto por el plan, envolviendo por los extremos, y devuelve la ruta
// que toca abrir. NULL si la cola esta vacia.
const char *Queue_Advance(SceBool forward);

// Salta a una pista que el usuario eligio. Con el barajado encendido la deja al
// frente de un plan nuevo. SCE_FALSE si esa ruta no esta en la cola, y entonces
// el plan no se toca.
SceBool Queue_SeekToPath(const char *path);

// La pista `n` puestos por delante en el plan, envolviendo; 0 es la que suena.
// `path` o `title` pueden ser NULL si no interesan, y el titulo sale NULL si el
// productor no trajo ninguno. SCE_FALSE si la cola esta vacia.
SceBool Queue_PeekAhead(int n, const char **path, const char **title);

// Productor de carpeta: vuelca en la cola los archivos reproducibles de esa
// carpeta, en el mismo orden en que se reproducian antes de existir el modulo.
int Queue_FillFromFolder(const char *dir);

#endif
