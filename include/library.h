#ifndef _ELEVENMPV_LIBRARY_H_
#define _ELEVENMPV_LIBRARY_H_

#include <psp2/types.h>

// La biblioteca: el indice de lo que hay bajo una carpeta elegida por el
// usuario, y el escaneo que lo construye.
//
// El indice es dato derivado y nunca fuente de verdad. La fuente son los
// archivos en disco, y siempre puede reconstruirse reescaneando; por eso su
// formato lleva version y una version que no se reconoce se descarta en vez de
// migrarse. La excepcion es la version anterior, que se carga con todas sus
// pistas pendientes de releer tags (ver Library_Load).

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
	// Numero de pista y de disco, 0 si el archivo no lo trae. Deciden el orden
	// dentro de un album.
	short track;
	short disc;
	// Si a esta pista ya se le leyeron los tags. Sin esta marca no se
	// podria distinguir "todavia no la mire" de "la mire y no traia nada",
	// que es justo lo que hace reanudable la segunda pasada.
	SceBool tagged;
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
// Library_Save escribe siempre la version actual. Library_Load acepta tambien
// la anterior, que no guarda numeros de pista: sus pistas se cargan con los
// tags que ya tenian pero marcadas como pendientes, y la pasada de tags, que
// el usuario lanza al reescanear, completa los numeros.
//
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

// --- las vistas ------------------------------------------------------------
// Una vista es un orden sobre el indice, no una copia: lo que se construye es
// una lista de indices de pista, o una lista de nombres distintos.
//
// "Desconocido" no esta en el indice. El indice guarda el artista y el album
// vacios cuando la pista no los trae, y la etiqueta y su posicion al final las
// pone la vista. Asi un artista que de verdad se llame "Desconocido" no se
// mezcla con el cubo, y el indice no queda escrito en un idioma.

typedef enum {
	LIBRARY_FIELD_ARTIST = 0,
	LIBRARY_FIELD_ALBUM
} Library_Field;

// Todas las pistas, por titulo.
int Library_BuildSongs(void);

// Todas las pistas, de la mas reciente a la mas antigua por fecha de
// modificacion. Empatan al segundo con frecuencia - una copia masiva desde el PC
// las deja todas iguales -, y ahi desempata la ruta, que es estable entre
// reescaneos y no cambia si el usuario edita un tag.
int Library_BuildRecent(void);

// Las pistas de ese nombre dentro de ese campo. Con `unknown`, las que no lo
// traen. Dentro de un album van en el orden del disco: disco, pista y titulo,
// con las sin numero al final. Dentro de un artista, agrupadas por album, con
// las sin album al final, y cada album en el orden del disco.
int Library_BuildFieldTracks(Library_Field field, const char *name, SceBool unknown);

// Toda la biblioteca agrupada por ese campo: los grupos en el orden de
// Library_BuildFieldNames, con el vacio al final, y dentro de cada grupo el
// orden de Library_BuildFieldTracks. Es la cola de "Seguir con el siguiente".
int Library_BuildContinuous(Library_Field field);

// Cuantas pistas tiene la vista construida, y cual es cada una.
int Library_ViewCount(void);
const Library_Track *Library_ViewTrack(int index);

// Los nombres distintos de ese campo, ordenados, con el cubo de los vacios al
// final si lo hay.
int Library_BuildFieldNames(Library_Field field);
int Library_NameCount(void);
const char *Library_NameAt(int index);
// SCE_TRUE para el cubo de las pistas que no traen ese campo.
SceBool Library_NameIsUnknown(int index);
int Library_NameTrackCount(int index);

// Los candidatos a caratula del nombre `index`, solo en la vista de artistas:
// una pista por album distinto, en el orden en que se ve el artista, mas cada
// pista sin album. La caratula del artista es la del primero que tenga una.
// NULL cuando `k` se pasa de la lista, y siempre para el cubo "Desconocido".
const Library_Track *Library_NameCandidate(int index, int k);

// Cuantas pistas del indice siguen sin tags leidos.
int Library_PendingTags(void);

// La segunda pasada: recorre el indice leyendo tags y persiste cada tantas
// pistas, de modo que abandonarla - o quedarse sin bateria - no tira lo
// leido y la siguiente continua por las que faltan.
//
// SCE_TRUE si no quedo ninguna pendiente.
SceBool Library_RunTagPass(void);

// La tercera pasada: extrae la caratula de cada album que aun no la tenga
// cacheada, y deja marcado tambien el "no hay" para no reintentarlo. Se apoya
// en que la clave de cache es el album, asi que la segunda pista de un disco
// ya la encuentra hecha.
//
// SCE_TRUE si termino, SCE_FALSE si el usuario la abandono.
SceBool Library_RunCoverPass(void);

#endif
