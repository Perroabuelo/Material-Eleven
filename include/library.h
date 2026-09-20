#ifndef _ELEVENMPV_LIBRARY_H_
#define _ELEVENMPV_LIBRARY_H_

#include <psp2/types.h>

// La biblioteca: el indice de lo que hay bajo una carpeta elegida por el
// usuario, y el escaneo que lo construye.
//
// El indice es dato derivado y nunca fuente de verdad. La fuente son los
// archivos en disco, y siempre puede reconstruirse reescaneando; por eso su
// formato lleva version y una version que no se reconoce se descarta en vez de
// migrarse.

// Lo que cuesta un registro en RAM: ruta 256, los tres campos de tags 64 cada
// uno, y tamaño, fecha y extension. Unos 472 bytes, que por el techo de abajo
// son ~1,9 MB con el indice lleno.
#define LIBRARY_PATH_MAX 256
#define LIBRARY_TAG_MAX  64
#define LIBRARY_EXT_MAX  8

// El techo lo fijo la medicion del grupo 0. No lo manda la memoria sino la
// espera: a los 159 ms por pista que costo leer tags en una coleccion real,
// 4000 pistas son unos once minutos de segunda pasada. Es soportable porque esa
// pasada es reanudable y porque la primera - la que deja la biblioteca
// utilizable - tarda cuatro segundos para esas mismas 4000.
#define LIBRARY_MAX_TRACKS 4000

// Un arbol elegido por el usuario puede ser arbitrariamente profundo, y llegar
// al tope deja esa rama sin explorar en vez de caerse.
#define LIBRARY_MAX_DEPTH 24

typedef struct {
	char path[LIBRARY_PATH_MAX];
	char title[LIBRARY_TAG_MAX];
	char artist[LIBRARY_TAG_MAX];
	char album[LIBRARY_TAG_MAX];
	SceOff size;
	SceUInt64 mtime;
	char ext[LIBRARY_EXT_MAX];
} Library_Track;

// --- la carpeta de escaneo -------------------------------------------------
// Vive en su propio archivo junto a lastdir.txt y no en config.cfg, porque
// Config_Load descarta el archivo entero cuando CONFIG_VERSION sube y se
// llevaria por delante los ajustes del usuario.

// Lee la carpeta recordada. Se llama una vez al arrancar.
void Library_LoadRoot(void);
SceBool Library_HasRoot(void);

// Si la carpeta recordada sigue estando. Puede no estarlo sin que sea culpa
// de nadie: el usuario retira la tarjeta. Entonces no se escanea ni se borra
// el indice que ya habia, solo se informa.
SceBool Library_RootAvailable(void);
const char *Library_GetRoot(void);

// Fija la carpeta y la recuerda. Descarta el indice de la anterior, para que no
// quede ninguna pista suya.
void Library_SetRoot(const char *path);

// --- el indice -------------------------------------------------------------
int Library_Count(void);
const Library_Track *Library_GetTrack(int index);

// Si el ultimo escaneo llego al techo y dejo pistas fuera.
SceBool Library_IsTruncated(void);

// Si hay un indice cargado, aunque este vacio: distingue "escanee y no habia
// nada" de "no he escaneado".
SceBool Library_IsBuilt(void);

void Library_Free(void);

// --- persistencia ----------------------------------------------------------
// SCE_FALSE si no hay indice, si su version no se reconoce o si esta
// incompleto. En los tres casos la biblioteca queda sin construir y lo que
// procede es ofrecer escanear, no enseñar datos parciales.
SceBool Library_Load(void);
SceBool Library_Save(void);

// --- el escaneo ------------------------------------------------------------
// Corre su propio lazo de fotogramas, con progreso dibujado y abandono, como ya
// hace Menu_PromptFilter mientras el teclado del sistema esta delante. La
// reproduccion en curso no se entera: el escaneo no toca el audio.
//
// SCE_TRUE si termino de recorrer, SCE_FALSE si el usuario lo abandono.
SceBool Library_RunScan(void);

#endif
