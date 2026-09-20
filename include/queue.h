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

// Vacia la cola y devuelve la posicion al principio.
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

int Queue_GetPosition(void);

// Sin acotar: quien llama decide que significa salirse por cada extremo, que
// no es lo mismo al avanzar que al repetir.
void Queue_SetPosition(int index);

// Indice de esa ruta exacta dentro de la cola, o 0 si no esta.
int Queue_IndexOf(const char *path);

// Productor de carpeta: vuelca en la cola los archivos reproducibles de esa
// carpeta, en el mismo orden en que se reproducian antes de existir el modulo.
int Queue_FillFromFolder(const char *dir);

#endif
