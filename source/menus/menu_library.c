#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "config.h"
#include "cover.h"
#include "fs.h"
#include "lang.h"
#include "library.h"
#include "list_filter.h"
#include "menu_audioplayer.h"
#include "menu_displayfiles.h"
#include "menu_library.h"
#include "menu_settings.h"
#include "mini_player.h"
#include "nav_rail.h"
#include "nav_request.h"
#include "playlist.h"
#include "playlist_store.h"
#include "queue.h"
#include "search_dialog.h"
#include "selection.h"
#include "status_bar.h"
#include "text_fold.h"
#include "touch.h"
#include "touch_list.h"
#include "ui_gpu.h"
#include "ui_theme.h"
#include "utils.h"

#define CONTENT_X   (UI_RAIL_WIDTH)
#define TOPBAR_H    64
#define ROW_H       64
#define ROW_ART     46
// Como en la lista de carpetas: el badge de formato termina a ROW_RIGHT_PAD
// del borde y el texto se detiene ROW_BADGE_GAP antes de el.
#define ROW_RIGHT_PAD 26
#define ROW_BADGE_GAP 16

// Cuantas miniaturas se admiten de disco por fotograma. Cada una son 64 KB de
// lectura; dos por fotograma llenan una pagina de cinco filas en tres
// fotogramas sin que el desplazamiento llegue a notarse detenido. Las que no
// llegan se dibujan con el marcador y aparecen en cuanto entran.
#define COVER_BUDGET_PER_FRAME 2
#define ROWS_PER_PAGE 5
#define LIST_TOP    (TOPBAR_H + 8)
// La lista se desplaza dentro de esta franja y se recorta en ella: no pisa la
// barra superior ni el contador de debajo. Caben cinco filas enteras.
#define LIST_BOTTOM (LIST_TOP + ROWS_PER_PAGE * ROW_H)

// La caja de busqueda de la barra superior, una pildora como la de carpetas
// pero mas estrecha: comparte la barra con el titulo y las cuatro pestañas,
// que en español son las mas anchas.
#define SEARCH_W       200
#define SEARCH_H       36
#define SEARCH_TAB_GAP 18.0f

// El menu de Cuadrado, anclado abajo a la derecha por encima del mini
// reproductor.
#define OPTIONS_W      280
#define OPTIONS_ITEM_H 56
#define OPTIONS_PAD    8

// Cuantos fotogramas se queda un aviso delante. A 60 fps son unos cuatro
// segundos: suficiente para leerlo, y se va solo sin pedir confirmacion.
#define NOTICE_FRAMES 240

// Cada cuantos fotogramas se vuelve a preguntar si la carpeta sigue ahi.
// A 60 fps, una vez por segundo.
#define ROOT_CHECK_PERIOD 60

// Las cinco formas de recorrer la coleccion, y la etiqueta de cada una.
typedef enum {
	VIEW_SONGS = 0,
	VIEW_ARTISTS,
	VIEW_ALBUMS,
	VIEW_RECENT,
	VIEW_PLAYLISTS,
	VIEW_COUNT
} Menu_LibraryTab;

static const LangString view_label[VIEW_COUNT] = { STR_VIEW_SONGS, STR_VIEW_ARTISTS, STR_VIEW_ALBUMS, STR_VIEW_RECENT, STR_VIEW_PLAYLISTS };

// La etiqueta del cubo de las pistas sin ese campo (STR_UNKNOWN). La pone la
// vista, no el indice: asi un artista que de verdad se llame asi no se mezcla con
// el cubo, y cambiar de idioma no obliga a reescanear.

static Menu_LibraryTab view = VIEW_SONGS;
static int selection = 0;

// El desplazamiento de la lista que se ve, en pixeles. Lo mueven el dedo y,
// para mostrar la seleccion, los botones.
static TouchList library_list;

// Dentro de un artista o de un album. El nombre vacio con `inside_unknown`
// puesto es el cubo.
static SceBool inside = SCE_FALSE;
static SceBool inside_unknown = SCE_FALSE;
static char inside_name[LIBRARY_TAG_MAX] = "";
static int outer_selection = 0;

// Los resumenes de la pestaña Playlists, de la modificada mas recientemente a
// la mas antigua. Se leen al entrar en la pestaña - o al abrir el selector de
// playlists - y se sueltan al salir de ella y de la pantalla; esta pantalla los
// rehace cada vez que escribe una playlist, y no en cada fotograma.
static PlaylistSummary *playlists = NULL;
static int playlist_count = 0;
static SceBool playlists_loaded = SCE_FALSE;

// Dentro de una playlist: su archivo, que es como se la vuelve a encontrar al
// volver a la pantalla, y sus canciones mientras se ven. La playlist completa
// solo esta en RAM mientras se esta dentro de ella y en esta pantalla.
static char open_file[PLAYLIST_FILE_MAX] = "";
static Playlist open_playlist;
static SceBool open_loaded = SCE_FALSE;

// Construir una vista es un qsort sobre miles de enteros. Se rehace cuando algo
// la invalida - cambiar de pestaña, entrar, salir, reescanear - y no en cada
// fotograma, que es lo que costaria dibujarla y leer el pad.
static SceBool view_dirty = SCE_TRUE;

// La lista que se ve es una capa de indices sobre la vista construida: una para
// la lista de la pestaña y otra para la de dentro de un grupo, porque volver de
// un grupo devuelve la de fuera tal como estaba. Toda la pantalla trabaja en
// posiciones visibles - `selection`, `outer_selection`, el dibujado y el
// contador - y traduce a la vista completa solo para llegar a una entrada.
_Static_assert(LIST_FILTER_MAX >= LIBRARY_MAX_TRACKS, "a ListFilter must hold the longest library view");

static ListFilter outer_filter;
static ListFilter group_filter;

// El termino de busqueda de cada lista, como se escribio - para mostrarlo y
// recargar el dialogo - y plegado una vez al aplicarlo, para comparar. Vacio
// es sin busqueda. Pertenece a la lista en que se escribio: cambiar de pestaña
// borra los dos, entrar en un grupo empieza el suyo vacio, y salir de la
// pantalla por el riel o por Reproduciendo no toca ninguno.
typedef struct {
	char term[64];
	char folded[64];
} Menu_LibrarySearch;

static Menu_LibrarySearch outer_search;
static Menu_LibrarySearch group_search;

// El menu de Cuadrado. Es un estado del mismo lazo, no un lazo propio:
// mientras esta abierto, la pantalla solo atiende al menu. Sus opciones se
// arman al abrirlo segun la fila marcada: primero lo que esa fila admite,
// despues "Seleccionar" donde hay modo seleccion, y al final reescanear y
// elegir carpeta, que estan siempre.
typedef enum {
	OPTION_ADD_TO_PLAYLIST = 0,
	OPTION_REMOVE_FROM_PLAYLIST,
	OPTION_MOVE,
	OPTION_RENAME,
	OPTION_DELETE,
	OPTION_SELECT,
	OPTION_RESCAN,
	OPTION_CHOOSE_FOLDER,
	OPTION_KINDS
} Menu_LibraryOption;

static const LangString option_label[OPTION_KINDS] = {
	STR_MENU_ADD_TO_PLAYLIST, STR_MENU_REMOVE_FROM_PLAYLIST, STR_MENU_MOVE, STR_MENU_RENAME, STR_MENU_DELETE,
	STR_MENU_SELECT, STR_MENU_RESCAN, STR_MENU_CHOOSE_FOLDER
};

// El caso mas largo es dentro de una playlist: quitar, mover, seleccionar,
// reescanear y elegir carpeta. Cinco filas caben entre la barra superior y el
// mini reproductor.
#define OPTIONS_MAX 5

static Menu_LibraryOption options[OPTIONS_MAX];
static int option_count = 0;
static SceBool options_open = SCE_FALSE;
static int options_index = 0;
// En el modo seleccion sin nada marcado la unica accion se ve apagada y no hace
// nada.
static SceBool options_disabled = SCE_FALSE;

// El selector de "Añadir a playlist…": "Nueva playlist…" y despues las
// playlists, en el orden de la pestaña. Es otro estado del mismo lazo, en el
// mismo recuadro que el menu, y se desplaza si no caben todas.
#define PICKER_W    360
#define PICKER_ROWS 5

// Mover una cancion dentro de la playlist abierta: otro estado del lazo. La
// cancion se desplaza en la copia en RAM y solo se guarda al confirmar;
// cancelar la devuelve a `move_origin`. Las dos son posiciones de la playlist
// completa, porque al mover no hay busqueda.
static SceBool moving = SCE_FALSE;
static int move_origin = 0;
static int move_at = 0;

// El dialogo que confirma borrar una playlist o quitar de la playlist abierta
// las canciones marcadas: una linea que dice que se hara y dos opciones, la
// accion y cancelar, con cancelar marcada al abrirse. Cada tipo pone su titulo
// y el nombre de su accion; la geometria, el toque y la leyenda son los mismos.
#define CONFIRM_ACTION 0
#define CONFIRM_CANCEL 1
#define CONFIRM_ITEMS  2

typedef enum {
	CONFIRM_DELETE_PLAYLIST,
	CONFIRM_REMOVE_MARKED
} Menu_LibraryConfirmKind;

static SceBool confirm_open = SCE_FALSE;
static Menu_LibraryConfirmKind confirm_kind = CONFIRM_DELETE_PLAYLIST;
static int confirm_index = CONFIRM_CANCEL;
static int confirm_count = 0;
static char confirm_file[PLAYLIST_FILE_MAX] = "";
static char confirm_name[PLAYLIST_NAME_MAX] = "";

static SceBool picker_open = SCE_FALSE;
static int picker_index = 0; // 0 es "Nueva playlist…", k la playlist k - 1
static int picker_top = 0;   // la primera opcion que se ve

// El modo seleccion: otro estado del lazo. Las marcas son posiciones de la
// lista completa del nivel vigente, asi que la busqueda no las toca. Mientras
// `header_focus` esta puesto la posicion actual es la casilla Todo, y
// `selection` guarda la ultima fila, que es adonde vuelve abajo.
static SceBool selecting = SCE_FALSE;
static Selection marks;
static SceBool header_focus = SCE_FALSE;

static void Menu_LibraryStartSelect(void) {
	selecting = SCE_TRUE;
	header_focus = SCE_FALSE;
	Selection_Clear(&marks);
}

// La unica salida del modo: descarta las marcas. La llama todo lo que cambia
// la lista completa, para que una marca nunca apunte a otra lista.
static void Menu_LibraryEndSelect(void) {
	selecting = SCE_FALSE;
	header_focus = SCE_FALSE;
	Selection_Clear(&marks);
}

// Declarada aqui porque elegir carpeta la usa y se define mas abajo, junto a
// las otras dos que mueven la vista.
static void Menu_LibrarySetView(Menu_LibraryTab next);
static SceBool root_missing = SCE_FALSE;
static int root_check_frames = 0;
// Un aviso fijo se guarda como id y se resuelve al dibujar, para que cambiar
// de idioma con uno en pantalla lo muestre ya traducido. Uno que lleva nombres
// o numeros se guarda ya escrito, en `notice_text`, con `notice` en STR_COUNT.
static LangString notice = STR_COUNT;
static char notice_text[192] = "";
static int notice_frames = 0;

// Como se llama una pista en la vista: su titulo si lo trae, y el nombre del
// archivo si no. El respaldo lo pone la vista y no el indice, para no guardar
// como dato algo que no se leyo de ningun tag.
static const char *Menu_LibraryTrackTitle(const Library_Track *track) {
	return (track->title[0] != '\0') ? track->title : Utils_Basename(track->path);
}

// Una vista es un orden sobre el indice, y se rehace cuando cambia algo: no se
// guarda entre fotogramas porque construirla cuesta un qsort sobre unos miles de
// enteros, y tenerla en cache obligaria a invalidarla en cada reescaneo.
static SceBool Menu_LibraryShowsNames(void) {
	return (view == VIEW_ARTISTS || view == VIEW_ALBUMS) && !inside;
}

// La lista de playlists de la pestaña, y no las canciones de una de ellas.
static SceBool Menu_LibraryShowsPlaylists(void) {
	return view == VIEW_PLAYLISTS && !inside;
}

// Si confirmar sobre una fila entra en ella en vez de reproducirla.
static SceBool Menu_LibraryRowsOpen(void) {
	return Menu_LibraryShowsNames() || Menu_LibraryShowsPlaylists();
}

static int Menu_LibraryLoadPlaylists(void) {
	if (!playlists_loaded) {
		int n = PlaylistStore_List(&playlists);

		// Sin memoria para la lista, la pestaña se ve vacia en vez de a medias.
		playlist_count = (n > 0) ? n : 0;
		playlists_loaded = SCE_TRUE;
	}

	return playlist_count;
}

static void Menu_LibraryFreePlaylists(void) {
	free(playlists);
	playlists = NULL;
	playlist_count = 0;
	playlists_loaded = SCE_FALSE;
}

static const PlaylistSummary *Menu_LibraryPlaylistAt(int full) {
	return (full >= 0 && full < playlist_count) ? &playlists[full] : NULL;
}

// La clave del mosaico de una playlist, o SCE_FALSE si no lleva mosaico.
static SceBool Menu_LibraryMosaicKey(const PlaylistSummary *s, char *key) {
	if (s->mosaic_count < 4)
		return SCE_FALSE;

	const char *albums[4], *paths[4];

	for (int q = 0; q < 4; q++) {
		albums[q] = s->mosaic_album[q];
		paths[q] = s->mosaic_path[q];
	}

	Cover_MosaicKey(albums, paths, key, COVER_MOSAIC_KEY_MAX);
	return SCE_TRUE;
}

// Vuelve a leer los resumenes despues de que esta pantalla escribio una
// playlist. El mosaico que una playlist deja de usar se borra, salvo que otra
// lo comparta: su clave depende de sus primeros albumes, asi que nadie mas lo
// volveria a pedir.
static void Menu_LibraryReloadPlaylists(void) {
	PlaylistSummary *old = playlists;
	int old_count = playlist_count;

	playlists = NULL;
	playlist_count = 0;
	playlists_loaded = SCE_FALSE;
	Menu_LibraryLoadPlaylists();

	for (int i = 0; i < old_count; i++) {
		char old_key[COVER_MOSAIC_KEY_MAX], key[COVER_MOSAIC_KEY_MAX];
		SceBool used = SCE_FALSE;

		if (!Menu_LibraryMosaicKey(&old[i], old_key))
			continue;

		for (int j = 0; j < playlist_count && !used; j++)
			used = Menu_LibraryMosaicKey(&playlists[j], key) && strcmp(key, old_key) == 0;

		if (!used)
			Cover_ForgetMosaic(old_key);
	}

	free(old);
}

// La caratula de una playlist: el mosaico con cuatro albumes, la del primero
// con uno a tres, y el marcador sin ninguno.
static vita2d_texture *Menu_LibraryPlaylistArt(const PlaylistSummary *s, int *budget) {
	if (s->mosaic_count >= 4) {
		const char *albums[4], *paths[4];

		for (int q = 0; q < 4; q++) {
			albums[q] = s->mosaic_album[q];
			paths[q] = s->mosaic_path[q];
		}

		return Cover_GetMosaic(albums, paths, !s->mosaic_pending, budget);
	}

	if (s->mosaic_count > 0)
		return Cover_Get(s->mosaic_album[0], s->mosaic_path[0], budget);

	return NULL;
}

// Las canciones de una playlist, y no la lista de playlists.
static SceBool Menu_LibraryInPlaylist(void) {
	return view == VIEW_PLAYLISTS && inside;
}

static void Menu_LibraryFreeOpenPlaylist(void) {
	Playlist_Free(&open_playlist);
	open_loaded = SCE_FALSE;
	Menu_LibraryEndSelect();
}

static const Playlist_Entry *Menu_LibraryEntryAt(int full) {
	return (open_loaded && full >= 0 && full < open_playlist.count) ? &open_playlist.entries[full] : NULL;
}

// Como se llama una cancion de la playlist en su fila: como en la biblioteca si
// la biblioteca la tiene, y si no con el titulo que se guardo al añadirla.
static const char *Menu_LibraryEntryTitle(const Playlist_Entry *e) {
	const Library_Track *t = Library_FindPath(e->path);

	if (t != NULL)
		return Menu_LibraryTrackTitle(t);

	return (e->title[0] != '\0') ? e->title : Utils_Basename(e->path);
}

// Vuelve a la pestaña sin entrar: la playlist que se tenia abierta ya no se
// puede leer, por ejemplo porque se borro mientras sonaba.
static void Menu_LibraryDropOpenPlaylist(void) {
	Menu_LibraryFreeOpenPlaylist();
	open_file[0] = '\0';
	inside = SCE_FALSE;
	group_search.term[0] = group_search.folded[0] = '\0';
	selection = outer_selection;
}

static Library_Field Menu_LibraryField(void) {
	return (view == VIEW_ALBUMS) ? LIBRARY_FIELD_ALBUM : LIBRARY_FIELD_ARTIST;
}

// La caratula de cada artista se elige al dibujar, probando sus candidatos de a
// uno y dentro del presupuesto del fotograma. Lo resuelto se guarda por nombre
// hasta que la vista se reconstruye; como los candidatos y su orden no cambian,
// al volver sale la misma.
#define ARTIST_COVER_PENDING -2
#define ARTIST_COVER_NONE    -1

static int *artist_cover = NULL;      // candidato elegido, o PENDING / NONE
static int *artist_cover_next = NULL; // el siguiente candidato a probar
static int artist_cover_count = 0;

static void Menu_LibraryResetArtistCovers(int count) {
	free(artist_cover);
	free(artist_cover_next);
	artist_cover = NULL;
	artist_cover_next = NULL;
	artist_cover_count = 0;

	if (count <= 0)
		return;

	artist_cover = (int *)malloc(sizeof(int) * (size_t)count);
	artist_cover_next = (int *)calloc((size_t)count, sizeof(int));

	if (artist_cover == NULL || artist_cover_next == NULL) {
		free(artist_cover);
		free(artist_cover_next);
		artist_cover = NULL;
		artist_cover_next = NULL;
		return;
	}

	for (int i = 0; i < count; i++)
		artist_cover[i] = ARTIST_COVER_PENDING;

	artist_cover_count = count;
}

// La pista cuya caratula representa al artista `i`, o NULL si todavia no se
// sabe o si no tiene ninguna. Cada candidato probado descuenta del presupuesto,
// asi que recorrer rapido una lista larga no se detiene.
static const Library_Track *Menu_LibraryArtistCover(int i, int *budget) {
	if (artist_cover == NULL || i < 0 || i >= artist_cover_count)
		return NULL;

	while (artist_cover[i] == ARTIST_COVER_PENDING) {
		const Library_Track *candidate = Library_NameCandidate(i, artist_cover_next[i]);

		if (candidate == NULL) {
			artist_cover[i] = ARTIST_COVER_NONE;
			break;
		}

		if (*budget <= 0)
			return NULL;

		(*budget)--;

		if (Cover_Probe(candidate->album, candidate->path) == COVER_PROBE_IMAGE)
			artist_cover[i] = artist_cover_next[i];
		else
			artist_cover_next[i]++;
	}

	return (artist_cover[i] >= 0) ? Library_NameCandidate(i, artist_cover[i]) : NULL;
}

// La lista visible vigente: la del grupo dentro de uno, la de la pestaña fuera.
static ListFilter *Menu_LibraryFilter(void) {
	return inside ? &group_filter : &outer_filter;
}

// La posicion en la vista completa de la fila visible `visible`, o -1.
static int Menu_LibraryFull(int visible) {
	return ListFilter_ToFull(Menu_LibraryFilter(), visible);
}

static int Menu_LibraryBuildFull(void) {
	// Al volver a la pantalla la playlist abierta se lee otra vez por su
	// archivo. Si ya no esta, la lista es la de la pestaña.
	if (Menu_LibraryInPlaylist() && !open_loaded) {
		if (PlaylistStore_Load(open_file, &open_playlist) == PLAYLIST_OK)
			open_loaded = SCE_TRUE;
		else
			Menu_LibraryDropOpenPlaylist();
	}

	if (Menu_LibraryInPlaylist())
		return open_playlist.count;

	if (Menu_LibraryShowsPlaylists())
		return Menu_LibraryLoadPlaylists();

	if (Menu_LibraryShowsNames()) {
		int names = Library_BuildFieldNames(Menu_LibraryField());
		Menu_LibraryResetArtistCovers((view == VIEW_ARTISTS) ? names : 0);
		return names;
	}

	if (inside)
		return Library_BuildFieldTracks(Menu_LibraryField(), inside_name, inside_unknown);

	if (view == VIEW_RECENT)
		return Library_BuildRecent();

	return Library_BuildSongs();
}

static Menu_LibrarySearch *Menu_LibrarySearchNow(void) {
	return inside ? &group_search : &outer_search;
}

static SceBool Menu_LibraryHasTerm(void) {
	return Menu_LibrarySearchNow()->term[0] != '\0';
}

// Que compara cada lista. El titulo es el que muestra la fila, con el nombre
// del archivo de respaldo, y el cubo se compara por la etiqueta que se ve.
static int Menu_LibraryMatchTrack(int index, void *folded) {
	const Library_Track *t = Library_ViewTrack(index);

	return t != NULL && (TextFold_Contains(Menu_LibraryTrackTitle(t), folded)
		|| TextFold_Contains(t->artist, folded) || TextFold_Contains(t->album, folded));
}

static int Menu_LibraryMatchName(int index, void *folded) {
	const char *name = Library_NameIsUnknown(index) ? Lang_Get(STR_UNKNOWN) : Library_NameAt(index);

	return name != NULL && TextFold_Contains(name, folded);
}

static int Menu_LibraryMatchTitle(int index, void *folded) {
	const Library_Track *t = Library_ViewTrack(index);

	return t != NULL && TextFold_Contains(Menu_LibraryTrackTitle(t), folded);
}

static int Menu_LibraryMatchEntry(int index, void *folded) {
	const Playlist_Entry *e = Menu_LibraryEntryAt(index);

	return e != NULL && TextFold_Contains(Menu_LibraryEntryTitle(e), folded);
}

static int Menu_LibraryMatchPlaylist(int index, void *folded) {
	const PlaylistSummary *s = Menu_LibraryPlaylistAt(index);

	return s != NULL && TextFold_Contains(s->name, folded);
}

// Cuantas entradas tiene la lista completa ya construida del nivel vigente.
static int Menu_LibraryFullCount(void) {
	if (Menu_LibraryInPlaylist())
		return open_playlist.count;

	if (Menu_LibraryShowsPlaylists())
		return playlist_count;

	return Menu_LibraryShowsNames() ? Library_NameCount() : Library_ViewCount();
}

// Rehace la lista visible sobre la vista ya construida, con el termino vigente.
static void Menu_LibraryRefilter(int full_count) {
	Menu_LibrarySearch *search = Menu_LibrarySearchNow();
	ListFilter_Match match = NULL;

	if (search->folded[0] != '\0' && Menu_LibraryShowsPlaylists())
		match = Menu_LibraryMatchPlaylist;
	else if (search->folded[0] != '\0' && Menu_LibraryInPlaylist())
		match = Menu_LibraryMatchEntry;
	else if (search->folded[0] != '\0')
		match = Menu_LibraryShowsNames() ? Menu_LibraryMatchName : (inside ? Menu_LibraryMatchTitle : Menu_LibraryMatchTrack);

	ListFilter_Build(Menu_LibraryFilter(), full_count, match, search->folded);
}

// Construye la vista si hace falta, y con ella la lista visible, y devuelve
// cuantas filas se ven. Reconstruir la vista - tambien despues de una cola
// continua - rehace la lista con el mismo termino, asi que vuelve igual.
static int Menu_LibraryBuild(void) {
	if (view_dirty) {
		view_dirty = SCE_FALSE;
		Menu_LibraryRefilter(Menu_LibraryBuildFull());
	}

	return ListFilter_Count(Menu_LibraryFilter());
}

// Pone el termino de la lista vigente, o lo quita con "", y rehace la lista
// visible sin reconstruir la vista. Con un termino nuevo la seleccion va al
// primer resultado; al quitarlo se queda en la misma entrada, que sigue ahi.
static void Menu_LibrarySetTerm(const char *term) {
	Menu_LibrarySearch *search = Menu_LibrarySearchNow();

	Menu_LibraryBuild();
	int full = Menu_LibraryFull(selection);

	snprintf(search->term, sizeof(search->term), "%s", term);
	TextFold_Fold(search->term, search->folded, sizeof(search->folded));

	Menu_LibraryRefilter(Menu_LibraryFullCount());

	int kept = (term[0] == '\0' && full >= 0) ? ListFilter_ToVisible(Menu_LibraryFilter(), full) : -1;
	selection = (kept >= 0) ? kept : 0;
	header_focus = SCE_FALSE;
	TouchList_Reset(&library_list);
	TouchList_Follow(&library_list);
}

static void Menu_LibraryClearSearches(void) {
	outer_search.term[0] = outer_search.folded[0] = '\0';
	group_search.term[0] = group_search.folded[0] = '\0';
}

static void Menu_LibraryNotice(LangString text) {
	notice = text;
	notice_text[0] = '\0';
	notice_frames = NOTICE_FRAMES;
}

// Un aviso con formato: `format` es un texto con especificadores de snprintf.
static void Menu_LibraryNoticef(LangString format, ...) {
	va_list args;

	va_start(args, format);
	vsnprintf(notice_text, sizeof(notice_text), Lang_Get(format), args);
	va_end(args);

	notice = STR_COUNT;
	notice_frames = NOTICE_FRAMES;
}

// Añade una frase al aviso con formato que se esta mostrando.
static void Menu_LibraryNoticeAppend(LangString format, ...) {
	size_t used = strlen(notice_text);
	va_list args;

	va_start(args, format);
	vsnprintf(notice_text + used, sizeof(notice_text) - used, Lang_Get(format), args);
	va_end(args);
}

// ---------------------------------------------------------------------------
// estado de la pantalla

typedef enum {
	LIBRARY_STATE_NO_ROOT = 0,  // no ha elegido carpeta nunca
	LIBRARY_STATE_UNBUILT,      // hay carpeta, pero no hay indice
	LIBRARY_STATE_EMPTY,        // escaneo hecho, carpeta sin musica
	LIBRARY_STATE_READY         // hay pistas
} Menu_LibraryState;

// Lo que dice cada estado vacio y la accion que ofrece, que es la de Confirmar.
static const LangString empty_line[LIBRARY_STATE_READY] = { STR_EMPTY_NO_ROOT, STR_EMPTY_UNBUILT, STR_EMPTY_NO_TRACKS };
static const LangString empty_action[LIBRARY_STATE_READY] = { STR_ACTION_CHOOSE_FOLDER, STR_ACTION_SCAN, STR_ACTION_CHOOSE_OTHER };

static Menu_LibraryState Menu_LibraryGetState(void) {
	if (!Library_HasRoot())
		return LIBRARY_STATE_NO_ROOT;

	if (!Library_IsBuilt())
		return LIBRARY_STATE_UNBUILT;

	if (Library_Count() == 0)
		return LIBRARY_STATE_EMPTY;

	return LIBRARY_STATE_READY;
}

// ---------------------------------------------------------------------------
// dibujado

// Donde empieza la primera de las cuatro pestañas, que se alinean a la derecha.
// Depende del idioma, asi que se mide en vez de fijarse.
static float Menu_LibraryTabsLeft(void) {
	float tab_x = 960 - 22;

	for (int i = VIEW_COUNT - 1; i >= 0; i--) {
		tab_x -= (float)UI_TextWidth(UI_FACE_MONO, UI_TS_BADGE, Lang_Get(view_label[i]));

		if (i > 0)
			tab_x -= SEARCH_TAB_GAP;
	}

	return tab_x;
}

// Donde empieza la pestaña `i` y cuanto mide. La comparten el dibujado y el
// toque.
static float Menu_LibraryTabX(int i, float *out_w) {
	float tab_x = 960 - 22;

	for (int j = VIEW_COUNT - 1; j >= i; j--) {
		float w = (float)UI_TextWidth(UI_FACE_MONO, UI_TS_BADGE, Lang_Get(view_label[j]));
		tab_x -= w;

		if (j == i) {
			*out_w = w;
			break;
		}

		tab_x -= SEARCH_TAB_GAP;
	}

	return tab_x;
}

// La franja de la barra en que se escriben el titulo y las pestañas.
#define TABS_BOX_H (TOPBAR_H - 18)

// Las casillas del modo seleccion: la de cada fila, a la izquierda de la
// caratula, y la de Todo en el encabezado, donde estaban las pestañas.
#define CHECK_SIZE   24
#define CHECK_GAP    14
#define HEADER_EXIT  24   // el aspa de salir
#define HEADER_GAP   16
#define HEADER_RIGHT 22   // margen derecho de la casilla Todo
#define HEADER_LABEL_GAP 10

// Una casilla de `size` en (x, y). `state` dice si esta vacia, parcial o llena;
// SELECTION_ALL_DISABLED la dibuja apagada.
static void Menu_DrawLibraryCheck(float x, float y, float size, Selection_All state) {
	float t = 2.0f;

	if (state == SELECTION_ALL_SOME || state == SELECTION_ALL_ALL) {
		UI_DrawRoundedRect(x, y, size, size, 6, ui_color_accent);

		if (state == SELECTION_ALL_ALL) {
			UI_DrawStroke(x + size * 0.24f, y + size * 0.52f, x + size * 0.43f, y + size * 0.70f, 3.0f, UI_COLOR_BG);
			UI_DrawStroke(x + size * 0.43f, y + size * 0.70f, x + size * 0.76f, y + size * 0.32f, 3.0f, UI_COLOR_BG);
		}
		else {
			UI_DrawStroke(x + size * 0.28f, y + size * 0.5f, x + size * 0.72f, y + size * 0.5f, 3.0f, UI_COLOR_BG);
		}

		return;
	}

	unsigned int color = (state == SELECTION_ALL_DISABLED) ? UI_COLOR_TEXT_MUTED : UI_COLOR_TEXT_SECONDARY;
	float l = x + t / 2, r = x + size - t / 2, top = y + t / 2, bottom = y + size - t / 2;

	UI_DrawStroke(l, top, r, top, t, color);
	UI_DrawStroke(r, top, r, bottom, t, color);
	UI_DrawStroke(r, bottom, l, bottom, t, color);
	UI_DrawStroke(l, bottom, l, top, t, color);
}

// Lo que se puede tocar en el encabezado: el aspa de salir a la izquierda y la
// casilla Todo, con su etiqueta, a la derecha. Lo comparten el dibujado y el
// toque.
static float Menu_LibraryHeaderExitX(void) { return CONTENT_X + 22; }
static float Menu_LibraryHeaderCheckX(void) { return 960 - HEADER_RIGHT - CHECK_SIZE; }

static float Menu_LibraryHeaderAllX(void) {
	return Menu_LibraryHeaderCheckX() - HEADER_LABEL_GAP - (float)UI_TextWidth(UI_FACE_UI, UI_TS_LABEL, Lang_Get(STR_SELECT_ALL));
}

// La caja de busqueda, a la izquierda de las pestañas. La comparten el
// dibujado y el toque.
static float Menu_LibrarySearchBoxX(void) { return Menu_LibraryTabsLeft() - SEARCH_TAB_GAP - SEARCH_W; }
static float Menu_LibrarySearchBoxY(void) { return (TOPBAR_H - SEARCH_H) / 2.0f; }

// La caja de busqueda, igual dentro y fuera del modo seleccion.
static void Menu_DrawLibrarySearchBox(void) {
	if (Menu_LibraryGetState() != LIBRARY_STATE_READY)
		return;

	float sx = Menu_LibrarySearchBoxX(), sy = Menu_LibrarySearchBoxY();
	SceBool has_term = Menu_LibraryHasTerm();

	UI_DrawPill(sx, sy, SEARCH_W, SEARCH_H, UI_COLOR_SURFACE);
	UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, sx + 14, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, sy, SEARCH_H),
		SEARCH_W - 28, has_term ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_MUTED,
		has_term ? Menu_LibrarySearchNow()->term : Lang_Get(STR_SEARCH_LIBRARY));
}

// En el modo seleccion la barra es un encabezado fijo: el aspa de salir, el
// contador de marcadas - tambien las que la busqueda oculta -, la caja de
// busqueda en su lugar y la casilla Todo, que mira solo lo visible. Las
// pestañas no se dibujan.
static void Menu_DrawLibraryHeader(void) {
	float cy = TOPBAR_H / 2.0f;
	float ex = Menu_LibraryHeaderExitX(), e = HEADER_EXIT * 0.36f;
	float ecx = ex + HEADER_EXIT / 2.0f;

	UI_DrawStroke(ecx - e, cy - e, ecx + e, cy + e, 3.0f, UI_COLOR_TEXT_PRIMARY);
	UI_DrawStroke(ecx - e, cy + e, ecx + e, cy - e, 3.0f, UI_COLOR_TEXT_PRIMARY);

	int n = Selection_Count(&marks);
	LangString format = Menu_LibraryShowsNames() ? ((n == 1) ? STR_SELECTED_GROUPS_ONE : STR_SELECTED_GROUPS_MANY)
		: ((n == 1) ? STR_SELECTED_SONGS_ONE : STR_SELECTED_SONGS_MANY);
	char counter[48];
	float x = ex + HEADER_EXIT + HEADER_GAP;

	snprintf(counter, sizeof(counter), Lang_Get(format), n);
	UI_DrawTextClipped(UI_FACE_UI, UI_TS_TITLE, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_TITLE, 0, TOPBAR_H),
		Menu_LibrarySearchBoxX() - 16 - x, UI_COLOR_TEXT_PRIMARY, counter);

	Menu_DrawLibrarySearchBox();

	Selection_All all = Selection_AllState(&marks, Menu_LibraryFilter());
	float cx = Menu_LibraryHeaderCheckX(), lx = Menu_LibraryHeaderAllX();

	// Marcada con el resalte de las filas cuando es la posicion actual.
	if (header_focus)
		UI_DrawRowHighlight(lx - 12, (TOPBAR_H - 44) / 2.0f, 960 - HEADER_RIGHT + 10 - (lx - 12), 44);

	UI_DrawText(UI_FACE_UI, UI_TS_LABEL, lx, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, 0, TOPBAR_H),
		(all == SELECTION_ALL_DISABLED) ? UI_COLOR_TEXT_MUTED : UI_COLOR_TEXT_PRIMARY, Lang_Get(STR_SELECT_ALL));
	Menu_DrawLibraryCheck(cx, (TOPBAR_H - CHECK_SIZE) / 2.0f, CHECK_SIZE, all);
}

static void Menu_DrawLibraryTopBar(void) {
	vita2d_draw_rectangle(CONTENT_X, 0, 960 - CONTENT_X, TOPBAR_H, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(CONTENT_X, TOPBAR_H - 1, 960 - CONTENT_X, 1, UI_COLOR_HAIRLINE);

	if (selecting) {
		Menu_DrawLibraryHeader();
		return;
	}

	float x = CONTENT_X + 22;
	float sx = Menu_LibrarySearchBoxX();
	// El titulo y la ruta se recortan antes de la caja en vez de pasarle por
	// debajo: dentro de un album el titulo es su nombre, y puede ser largo.
	float text_w = sx - 16 - x;

	// Dentro de un artista o un album manda su nombre, porque es lo que el usuario
	// acaba de elegir; la pestaña sigue marcada al lado.
	const char *heading = Menu_LibraryInPlaylist() ? open_playlist.name
		: inside ? (inside_unknown ? Lang_Get(STR_UNKNOWN) : inside_name) : Lang_Get(STR_LIBRARY_TITLE);

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_TITLE, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_TITLE, 0, TOPBAR_H - 18),
		text_w, UI_COLOR_TEXT_PRIMARY, heading);

	// Las cuatro pestañas, con la vigente marcada. Se dibujan tambien dentro de un
	// grupo, para que se vea de donde se entro.
	for (int i = 0; i < VIEW_COUNT; i++) {
		float w = 0;
		float tab_x = Menu_LibraryTabX(i, &w);

		UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, tab_x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, 0, TABS_BOX_H),
			(i == view) ? ui_color_accent : UI_COLOR_TEXT_MUTED, Lang_Get(view_label[i]));
	}

	if (Library_HasRoot()) {
		UI_DrawTextClipped(UI_FACE_MONO, UI_TS_BADGE, x,
			UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, TOPBAR_H - 26, 20), text_w,
			UI_COLOR_TEXT_TERTIARY, Library_GetRoot());
	}

	// La caja solo se ofrece cuando hay algo en que buscar. Muestra el termino
	// de la lista que se ve, o la invitacion a buscar.
	Menu_DrawLibrarySearchBox();
}

// Un estado vacio que dice cual de los cuatro es y que hacer, en vez de una
// lista vacia sin explicacion. La accion se puede tocar.
#define PLACEHOLDER_X (CONTENT_X + 22)
#define PLACEHOLDER_Y 180.0f
#define PLACEHOLDER_ACTION_Y (PLACEHOLDER_Y + 34)
#define PLACEHOLDER_ACTION_H 24

static void Menu_DrawLibraryPlaceholder(const char *line, const char *action) {
	float x = PLACEHOLDER_X, y = PLACEHOLDER_Y;

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, 26),
		960 - x - 22, UI_COLOR_TEXT_SECONDARY, line);

	if (action != NULL) {
		UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, PLACEHOLDER_ACTION_Y, PLACEHOLDER_ACTION_H),
			UI_COLOR_TEXT_TERTIARY, action);
	}
}

// Una fila: la linea de arriba es lo que la vista lista, y la de abajo lo que
// ayuda a distinguirlo de sus vecinas.
// `art` es la caratula, o NULL para el marcador. `ext` es la extension del
// archivo en las filas que son una cancion, y NULL en las de artista, album o
// playlist, que no son un archivo y no llevan badge de formato. Una fila
// `dimmed` es una cancion de playlist que la biblioteca ya no tiene.
static void Menu_DrawLibraryRow(int i, float y, const char *primary, const char *secondary,
		vita2d_texture *art, const char *ext, SceBool dimmed) {
	// Con la casilla Todo como posicion actual, ninguna fila se ve marcada.
	if (i == selection && !header_focus)
		UI_DrawRowHighlight(CONTENT_X, y, 960 - CONTENT_X, ROW_H);
	else if (i == TouchList_PressedRow(&library_list))
		UI_DrawRowPressHighlight(CONTENT_X, y, 960 - CONTENT_X, ROW_H);

	// La cancion que se esta moviendo lleva ademas una marca del acento.
	if (moving && i == selection)
		vita2d_draw_rectangle(CONTENT_X, y, 4, ROW_H, ui_color_accent);

	float art_x = CONTENT_X + 22, art_y = y + (ROW_H - ROW_ART) / 2.0f;

	// En el modo seleccion la casilla va antes de la caratula, y la fila se
	// corre su ancho: el texto y el badge se recortan como siempre.
	if (selecting) {
		Menu_DrawLibraryCheck(art_x, y + (ROW_H - CHECK_SIZE) / 2.0f, CHECK_SIZE,
			Selection_Has(&marks, Menu_LibraryFull(i)) ? SELECTION_ALL_ALL : SELECTION_ALL_NONE);
		art_x += CHECK_SIZE + CHECK_GAP;
	}

	if (art != NULL && dimmed) {
		vita2d_draw_texture_tint_scale(art, art_x, art_y, ROW_ART / (float)COVER_SIZE, ROW_ART / (float)COVER_SIZE,
			RGBA8(255, 255, 255, 96));
	}
	else if (art != NULL) {
		vita2d_draw_texture_scale(art, art_x, art_y, ROW_ART / (float)COVER_SIZE, ROW_ART / (float)COVER_SIZE);
	}
	else {
		// El marcador por defecto, coherente con el resto de la interfaz. Es lo que
		// se ve cuando no hay caratula y tambien mientras una que si hay todavia no
		// ha llegado de disco.
		UI_DrawRoundedRect(art_x, art_y, ROW_ART, ROW_ART, 12, UI_COLOR_SURFACE_2);
	}

	float x = art_x + ROW_ART + 14;
	float text_w = 960 - x - 100;

	// El badge termina donde termina el de la lista de carpetas, y el texto se
	// recorta antes de llegar a el en vez de pasarle por debajo.
	// Una fila atenuada no lleva badge: el archivo que diria su formato no esta.
	const char *badge_label = NULL; unsigned int badge_color = 0, badge_wash = 0;
	SceBool has_badge = (ext != NULL) && !dimmed && UI_GetFormatBadge(ext, &badge_label, &badge_color, &badge_wash);
	float badge_x = 0;

	if (has_badge) {
		badge_x = 960 - ROW_RIGHT_PAD - UI_BadgeWidth(UI_TS_BADGE, badge_label);
		text_w = badge_x - ROW_BADGE_GAP - x;
	}

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y + 6, 24),
		text_w, dimmed ? UI_COLOR_TEXT_TERTIARY : UI_COLOR_TEXT_PRIMARY, primary);

	if (secondary != NULL) {
		UI_DrawTextClipped(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y + 34, 20),
			text_w, dimmed ? UI_COLOR_TEXT_MUTED : UI_COLOR_TEXT_TERTIARY, secondary);
	}

	if (has_badge)
		UI_DrawBadge(badge_x, y + (ROW_H - 30) / 2, UI_TS_BADGE, badge_label, badge_wash, badge_color);
}

static void Menu_DrawLibraryList(void) {
	int count = Menu_LibraryBuild();

	// Una busqueda que no deja nada lo dice, en vez de una lista vacia.
	if (count == 0 && Menu_LibraryHasTerm()) {
		char line[160];
		snprintf(line, sizeof(line), Lang_Get(STR_NO_RESULTS), Menu_LibrarySearchNow()->term);
		Menu_DrawLibraryPlaceholder(line, NULL);
		return;
	}

	// Sin playlists, la pestaña dice como se crea una; una playlist vacia lo dice.
	if (count == 0 && Menu_LibraryShowsPlaylists()) {
		Menu_DrawLibraryPlaceholder(Lang_Get(STR_PLAYLISTS_EMPTY), Lang_Get(STR_PLAYLISTS_EMPTY_HOW));
		return;
	}

	if (count == 0 && Menu_LibraryInPlaylist()) {
		Menu_DrawLibraryPlaceholder(Lang_Get(STR_PLAYLIST_EMPTY), NULL);
		return;
	}

	TouchList_Layout(&library_list, CONTENT_X, LIST_TOP, 960, LIST_BOTTOM, count, ROW_H, selection);
	TouchList_BeginClip(&library_list);

	int budget = COVER_BUDGET_PER_FRAME;

	// `i` es la fila visible, que es la que se marca; `full`, la entrada que
	// muestra. Se dibujan las que asoman, aunque sea en parte.
	for (int i = TouchList_FirstRow(&library_list); i < count; i++) {
		float y = TouchList_RowY(&library_list, i);

		if (y >= LIST_BOTTOM)
			break;

		int full = Menu_LibraryFull(i);

		if (Menu_LibraryInPlaylist()) {
			const Playlist_Entry *e = Menu_LibraryEntryAt(full);

			if (e == NULL)
				break;

			// Como la de un album si la biblioteca la tiene: sus tags, su
			// extension y su caratula. Si no, atenuada, con lo que se guardo.
			const Library_Track *t = Library_FindPath(e->path);
			const char *artist = (t != NULL) ? t->artist : e->artist;

			if (artist[0] == '\0')
				artist = Lang_Get(STR_UNKNOWN);

			Menu_DrawLibraryRow(i, y, Menu_LibraryEntryTitle(e), artist,
				Cover_Get((t != NULL) ? t->album : e->album, e->path, &budget),
				(t != NULL) ? t->ext : FS_GetFileExt(e->path), t == NULL);
		}
		else if (Menu_LibraryShowsPlaylists()) {
			const PlaylistSummary *s = Menu_LibraryPlaylistAt(full);

			if (s == NULL)
				break;

			char sub[48];
			snprintf(sub, sizeof(sub), Lang_Get((s->count == 1) ? STR_TRACKS_ONE : STR_TRACKS_MANY), s->count);
			Menu_DrawLibraryRow(i, y, s->name, sub, Menu_LibraryPlaylistArt(s, &budget), NULL, SCE_FALSE);
		}
		else if (Menu_LibraryShowsNames()) {
			char sub[48];
			int n = Library_NameTrackCount(full);

			snprintf(sub, sizeof(sub), Lang_Get((n == 1) ? STR_TRACKS_ONE : STR_TRACKS_MANY), n);

			// En la vista de albumes la fila ES un album, asi que su caratula es la
			// del cubo. En la de artistas es la de la primera de sus pistas que
			// tenga una, y el cubo "Desconocido" se queda con el marcador.
			const char *art_album = NULL, *art_path = NULL;

			if (view == VIEW_ALBUMS && !Library_NameIsUnknown(full)) {
				art_album = Library_NameAt(full);
			}
			else if (view == VIEW_ARTISTS) {
				const Library_Track *rep = Menu_LibraryArtistCover(full, &budget);

				if (rep != NULL) {
					art_album = rep->album;
					art_path = rep->path;
				}
			}

			vita2d_texture *art = (art_album != NULL || art_path != NULL) ? Cover_Get(art_album, art_path, &budget) : NULL;
			Menu_DrawLibraryRow(i, y, Library_NameIsUnknown(full) ? Lang_Get(STR_UNKNOWN) : Library_NameAt(full), sub,
				art, NULL, SCE_FALSE);
		}
		else {
			const Library_Track *track = Library_ViewTrack(full);

			if (track == NULL)
				break;

			const char *artist = (track->artist[0] != '\0') ? track->artist : Lang_Get(STR_UNKNOWN);
			Menu_DrawLibraryRow(i, y, Menu_LibraryTrackTitle(track), artist, Cover_Get(track->album, track->path, &budget),
				track->ext, SCE_FALSE);
		}
	}

	TouchList_EndClip();

	char counter[96];

	int pending = Library_PendingTags();

	if (Library_IsTruncated())
		snprintf(counter, sizeof(counter), Lang_Get(STR_COUNTER_TRUNCATED), selection + 1, count);
	else if (pending > 0)
		snprintf(counter, sizeof(counter), Lang_Get((pending == 1) ? STR_COUNTER_PENDING_ONE : STR_COUNTER_PENDING_MANY), selection + 1, count, pending);
	else
		snprintf(counter, sizeof(counter), Lang_Get(STR_COUNTER), selection + 1, count);

	// Justo debajo de la ultima fila y por encima del mini reproductor, que
	// empieza en MiniPlayer_Top(): asi el contador no queda tapado cuando hay
	// algo sonando ni suelto en medio cuando no.
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, CONTENT_X + 22,
		UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, LIST_BOTTOM + 4, 22),
		UI_COLOR_TEXT_MUTED, counter);
}

#define NOTICE_H 44.0f

static float Menu_LibraryNoticeY(void) { return MiniPlayer_Top() - NOTICE_H - 8; }

static SceBool Menu_LibraryNoticeShown(void) {
	return notice_frames > 0 && (notice < STR_COUNT || notice_text[0] != '\0');
}

static void Menu_DrawLibraryNotice(void) {
	if (!Menu_LibraryNoticeShown())
		return;

	float h = NOTICE_H, y = Menu_LibraryNoticeY();

	UI_DrawRoundedRect(CONTENT_X + 16, y, 960 - CONTENT_X - 32, h, 12, UI_COLOR_SURFACE_2);
	UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, CONTENT_X + 32, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, y, h),
		960 - CONTENT_X - 64, UI_COLOR_TEXT_PRIMARY, (notice < STR_COUNT) ? Lang_Get(notice) : notice_text);
}

static float Menu_LibraryOptionsH(void) { return option_count * OPTIONS_ITEM_H + 2 * OPTIONS_PAD; }
static float Menu_LibraryOptionsX(void) { return 960 - 16 - OPTIONS_W; }
static float Menu_LibraryOptionsY(void) { return MiniPlayer_Top() - 8 - Menu_LibraryOptionsH(); }

static float Menu_LibraryOptionY(int i) { return Menu_LibraryOptionsY() + OPTIONS_PAD + i * OPTIONS_ITEM_H; }

static void Menu_DrawLibraryOptions(void) {
	if (!options_open)
		return;

	float x = Menu_LibraryOptionsX();

	UI_DrawRoundedRect(x, Menu_LibraryOptionsY(), OPTIONS_W, Menu_LibraryOptionsH(), 14, UI_COLOR_SURFACE_2);

	for (int i = 0; i < option_count; i++) {
		float y = Menu_LibraryOptionY(i);

		if (i == options_index)
			UI_DrawRowHighlight(x + OPTIONS_PAD, y, OPTIONS_W - 2 * OPTIONS_PAD, OPTIONS_ITEM_H);

		UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x + OPTIONS_PAD + 16, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, OPTIONS_ITEM_H),
			OPTIONS_W - 2 * OPTIONS_PAD - 32, options_disabled ? UI_COLOR_TEXT_SECONDARY : UI_COLOR_TEXT_PRIMARY,
			Lang_Get(option_label[options[i]]));
	}
}

static int Menu_LibraryPickerCount(void) { return 1 + playlist_count; }

static int Menu_LibraryPickerShown(void) {
	int n = Menu_LibraryPickerCount();
	return (n < PICKER_ROWS) ? n : PICKER_ROWS;
}

static float Menu_LibraryPickerH(void) { return Menu_LibraryPickerShown() * OPTIONS_ITEM_H + 2 * OPTIONS_PAD; }
static float Menu_LibraryPickerX(void) { return 960 - 16 - PICKER_W; }
static float Menu_LibraryPickerY(void) { return MiniPlayer_Top() - 8 - Menu_LibraryPickerH(); }

// La fila visible `row` del selector.
static float Menu_LibraryPickerRowY(int row) { return Menu_LibraryPickerY() + OPTIONS_PAD + row * OPTIONS_ITEM_H; }

static void Menu_DrawLibraryPicker(void) {
	if (!picker_open)
		return;

	float x = Menu_LibraryPickerX();

	UI_DrawRoundedRect(x, Menu_LibraryPickerY(), PICKER_W, Menu_LibraryPickerH(), 14, UI_COLOR_SURFACE_2);

	for (int row = 0; row < Menu_LibraryPickerShown(); row++) {
		int item = picker_top + row;
		float y = Menu_LibraryPickerRowY(row);

		if (item == picker_index)
			UI_DrawRowHighlight(x + OPTIONS_PAD, y, PICKER_W - 2 * OPTIONS_PAD, OPTIONS_ITEM_H);

		const PlaylistSummary *s = Menu_LibraryPlaylistAt(item - 1);
		const char *label = (item == 0) ? Lang_Get(STR_MENU_NEW_PLAYLIST) : (s != NULL ? s->name : "");

		UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x + OPTIONS_PAD + 16, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, OPTIONS_ITEM_H),
			PICKER_W - 2 * OPTIONS_PAD - 32, (item == 0) ? ui_color_accent : UI_COLOR_TEXT_PRIMARY, label);
	}
}

static float Menu_LibraryConfirmH(void) { return (CONFIRM_ITEMS + 1) * OPTIONS_ITEM_H + 2 * OPTIONS_PAD; }
static float Menu_LibraryConfirmX(void) { return 960 - 16 - PICKER_W; }
static float Menu_LibraryConfirmY(void) { return MiniPlayer_Top() - 8 - Menu_LibraryConfirmH(); }

// La opcion `i`, debajo de la linea del titulo.
static float Menu_LibraryConfirmItemY(int i) { return Menu_LibraryConfirmY() + OPTIONS_PAD + (i + 1) * OPTIONS_ITEM_H; }

static void Menu_DrawLibraryConfirm(void) {
	if (!confirm_open)
		return;

	float x = Menu_LibraryConfirmX(), y = Menu_LibraryConfirmY();
	char title[PLAYLIST_NAME_MAX + 64];
	SceBool removing = (confirm_kind == CONFIRM_REMOVE_MARKED);
	const LangString item_label[CONFIRM_ITEMS] = { removing ? STR_MENU_REMOVE : STR_MENU_DELETE, STR_HINT_CANCEL };

	if (removing)
		snprintf(title, sizeof(title), Lang_Get((confirm_count == 1) ? STR_REMOVE_CONFIRM_ONE : STR_REMOVE_CONFIRM_MANY),
			confirm_count, confirm_name);
	else
		snprintf(title, sizeof(title), Lang_Get(STR_DELETE_CONFIRM), confirm_name);
	UI_DrawRoundedRect(x, y, PICKER_W, Menu_LibraryConfirmH(), 14, UI_COLOR_SURFACE_2);
	UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x + OPTIONS_PAD + 16, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y + OPTIONS_PAD, OPTIONS_ITEM_H),
		PICKER_W - 2 * OPTIONS_PAD - 32, UI_COLOR_TEXT_SECONDARY, title);

	for (int i = 0; i < CONFIRM_ITEMS; i++) {
		float iy = Menu_LibraryConfirmItemY(i);

		if (i == confirm_index)
			UI_DrawRowHighlight(x + OPTIONS_PAD, iy, PICKER_W - 2 * OPTIONS_PAD, OPTIONS_ITEM_H);

		UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x + OPTIONS_PAD + 16, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, iy, OPTIONS_ITEM_H),
			PICKER_W - 2 * OPTIONS_PAD - 32, UI_COLOR_TEXT_PRIMARY, Lang_Get(item_label[i]));
	}
}

// El cuerpo de la pantalla, sin leyenda ni riel: lo comparten el lazo normal y
// el fondo del dialogo de busqueda, que deben mostrar lo mismo.
static void Menu_DrawLibraryContent(void) {
	StatusBar_Display();
	Menu_DrawLibraryTopBar();

	// La comprobacion es una llamada al sistema de archivos, asi que se hace
	// cada tantos fotogramas y no en todos.
	if ((root_check_frames--) <= 0) {
		root_check_frames = ROOT_CHECK_PERIOD;
		root_missing = Library_HasRoot() && !Library_RootAvailable();
	}

	if (root_missing) {
		UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, CONTENT_X + 22,
			UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, TOPBAR_H + 10, 24), 960 - CONTENT_X - 44,
			UI_COLOR_TEXT_SECONDARY, Lang_Get(STR_ROOT_MISSING));
	}

	Menu_LibraryState state = Menu_LibraryGetState();

	if (state == LIBRARY_STATE_READY)
		Menu_DrawLibraryList();
	else
		Menu_DrawLibraryPlaceholder(Lang_Get(empty_line[state]), Lang_Get(empty_action[state]));

	MiniPlayer_Draw();
	Menu_DrawLibraryNotice();
	Menu_DrawLibraryOptions();
	Menu_DrawLibraryPicker();
	Menu_DrawLibraryConfirm();
}

// El termino cambia solo con uno confirmado: descartar, abandonar o no poder
// abrir el dialogo dejan la lista con el que tenia. Confirmar vacio lo quita.
static void Menu_LibraryPromptSearch(void) {
	char term[64];

	if (SearchDialog_Prompt(Lang_Get(STR_SEARCH_LIBRARY), Menu_LibrarySearchNow()->term, term, sizeof(term),
		Menu_DrawLibraryContent, UI_SCREEN_LIBRARY) != SEARCH_DIALOG_CONFIRMED)
		return;

	Menu_LibrarySetTerm(term);
}

// ---------------------------------------------------------------------------
// acciones

static void Menu_LibraryPickRoot(void) {
	char picked[LIBRARY_PATH_MAX];

	if (!Menu_PickFolder(picked, sizeof(picked)))
		return;

	Library_SetRoot(picked);
	selection = 0;
	Menu_LibrarySetView(VIEW_SONGS);

	// La primera pasada deja la biblioteca utilizable; la segunda la completa y
	// la tercera la ilustra. Encadenadas, pero separadas: abandonar una no tira
	// lo que hicieron las anteriores.
	if (Library_RunScan() && Library_RunTagPass())
		Library_RunCoverPass();

	if (Library_IsTruncated())
		Menu_LibraryNotice(STR_NOTICE_TRUNCATED);
}

static void Menu_LibraryRescan(void) {
	Menu_LibraryEndSelect();

	if (!Library_HasRoot())
		return;

	view_dirty = SCE_TRUE;

	if (!Library_RootAvailable()) {
		Menu_LibraryNotice(STR_NOTICE_ROOT_GONE);
		return;
	}

	// Lo escaneado puede no tener ya el grupo ni los resultados que se veian:
	// la misma pestaña, desde arriba, sin busqueda.
	Menu_LibrarySetView(view);

	if (Library_RunScan() && Library_RunTagPass())
		Library_RunCoverPass();

	if (Library_IsTruncated())
		Menu_LibraryNotice(STR_NOTICE_TRUNCATED);
}

// El productor de biblioteca: vuelca la vista vigente en la cola, en el orden
// en que se ve, y reproduce desde ahi. Siguiente y anterior recorren esa cola y
// no la carpeta en la que este el archivo.
//
// Dentro de un album o un artista, con "Seguir con el siguiente", la cola es
// en cambio toda la biblioteca agrupada por ese campo. Esa vista comparte el
// arreglo con la que se esta viendo, asi que el orden de los pasos importa:
// la pista elegida se toma antes de construirla, y la vista visible se marca
// para reconstruirse despues.
static void Menu_LibraryPlaySelected(void) {
	// La cola es esta vista, en el orden en que se ve. Por eso se reconstruye
	// aqui: es lo que hace que reproducir desde un album encadene el album y no
	// la carpeta en la que este cada archivo.
	Menu_LibraryBuild();

	// La fila elegida se traduce a la vista completa, y la cola es esa vista
	// completa: una busqueda decide que pista se elige, no que suena despues.
	const Library_Track *track = Library_ViewTrack(Menu_LibraryFull(selection));

	if (track == NULL)
		return;

	// El indice es una cache de lo que habia en disco, y el disco pudo cambiar
	// desde el ultimo escaneo. Comprobarlo aqui es barato; validar el indice
	// entero al arrancar seria un escaneo.
	if (!FS_FileExists(track->path)) {
		Menu_LibraryNotice(STR_NOTICE_FILE_GONE);
		return;
	}

	// La pista apunta al indice, no a la vista: sigue valiendo despues de
	// construir otra.
	SceBool continuous = inside && (config.group_end == CONFIG_GROUP_END_NEXT);

	if (continuous)
		Library_BuildContinuous(Menu_LibraryField());

	Queue_Clear();

	for (int i = 0; i < Library_ViewCount(); i++) {
		const Library_Track *t = Library_ViewTrack(i);

		if (t == NULL)
			break;

		if (!Queue_Add(t->path, Menu_LibraryTrackTitle(t), t->album))
			break;
	}

	// El siguiente fotograma vuelve a la lista del grupo, no a la biblioteca
	// entera.
	if (continuous)
		view_dirty = SCE_TRUE;

	// La biblioteca indexa por extension sin abrir nada, asi que lista tambien
	// lo que no se puede decodificar. Decirlo es mejor que no hacer nada.
	if (!Menu_PlayQueued(track->path))
		Menu_LibraryNotice(STR_NOTICE_UNREADABLE);
}

// ---------------------------------------------------------------------------

// Reproducir desde una playlist: la cola es la playlist entera, en su orden, y
// el ajuste de fin de album no la toca. La cola copia cada cadena, asi que es
// una foto: lo que se edite despues en la playlist no cambia lo que suena.
static void Menu_LibraryPlayPlaylist(void) {
	Menu_LibraryBuild();

	const Playlist_Entry *chosen = Menu_LibraryEntryAt(Menu_LibraryFull(selection));

	if (chosen == NULL)
		return;

	if (!FS_FileExists(chosen->path)) {
		Menu_LibraryNotice(STR_NOTICE_FILE_GONE);
		return;
	}

	Queue_Clear();

	for (int i = 0; i < open_playlist.count; i++) {
		const Playlist_Entry *e = &open_playlist.entries[i];
		const Library_Track *t = Library_FindPath(e->path);

		// El album, del indice o de #EXTALB, para que "A continuacion" muestre
		// su caratula aunque la biblioteca ya no tenga la pista.
		if (!Queue_Add(e->path, Menu_LibraryEntryTitle(e), (t != NULL) ? t->album : e->album))
			break;
	}

	if (!Menu_PlayQueued(chosen->path))
		Menu_LibraryNotice(STR_NOTICE_UNREADABLE);
}

static void Menu_LibraryEnterPlaylist(void) {
	const PlaylistSummary *s = Menu_LibraryPlaylistAt(Menu_LibraryFull(selection));
	Playlist pl;

	if (s == NULL)
		return;

	if (PlaylistStore_Load(s->file, &pl) != PLAYLIST_OK) {
		Menu_LibraryNotice(STR_NOTICE_PLAYLIST_UNREADABLE);
		return;
	}

	Menu_LibraryFreeOpenPlaylist();
	open_playlist = pl;
	open_loaded = SCE_TRUE;
	snprintf(open_file, sizeof(open_file), "%s", s->file);

	outer_selection = selection;
	inside = SCE_TRUE;
	group_search.term[0] = group_search.folded[0] = '\0';
	selection = 0;
	view_dirty = SCE_TRUE;
	TouchList_Reset(&library_list);
}

static void Menu_LibraryEnter(void) {
	int full = Menu_LibraryFull(selection);
	const char *name = Library_NameAt(full);

	if (name == NULL)
		return;

	Menu_LibraryEndSelect();
	inside_unknown = Library_NameIsUnknown(full);
	snprintf(inside_name, sizeof(inside_name), "%s", name);
	outer_selection = selection;
	inside = SCE_TRUE;
	group_search.term[0] = group_search.folded[0] = '\0';
	selection = 0;
	view_dirty = SCE_TRUE;
	TouchList_Reset(&library_list);
}

// SCE_TRUE si habia de donde salir. La lista de fuera vuelve con su termino, y
// `outer_selection` sigue valiendo porque ese termino no cambio. Al salir de una
// playlist que se modifico, esta paso al principio de la pestaña: la seleccion
// la sigue.
static SceBool Menu_LibraryLeave(void) {
	if (!inside)
		return SCE_FALSE;

	Menu_LibraryEndSelect();

	char was_open[PLAYLIST_FILE_MAX];
	snprintf(was_open, sizeof(was_open), "%s", open_file);

	inside = SCE_FALSE;
	group_search.term[0] = group_search.folded[0] = '\0';
	inside_unknown = SCE_FALSE;
	inside_name[0] = '\0';
	Menu_LibraryFreeOpenPlaylist();
	open_file[0] = '\0';
	selection = outer_selection;
	view_dirty = SCE_TRUE;

	if (was_open[0] != '\0') {
		int count = Menu_LibraryBuild();

		for (int v = 0; v < count; v++) {
			const PlaylistSummary *s = Menu_LibraryPlaylistAt(Menu_LibraryFull(v));

			if (s != NULL && strcmp(s->file, was_open) == 0) {
				selection = v;
				break;
			}
		}

		if (selection >= count)
			selection = (count > 0) ? count - 1 : 0;
	}

	TouchList_Reset(&library_list);
	TouchList_Follow(&library_list);
	return SCE_TRUE;
}

static void Menu_LibrarySetView(Menu_LibraryTab next) {
	// Los resumenes son de la pestaña: se sueltan al dejarla, y al volver a
	// ella se leen de nuevo.
	Menu_LibraryEndSelect();
	Menu_LibraryFreePlaylists();
	Menu_LibraryFreeOpenPlaylist();
	open_file[0] = '\0';
	view = next;
	inside = SCE_FALSE;
	inside_unknown = SCE_FALSE;
	inside_name[0] = '\0';
	selection = 0;
	outer_selection = 0;
	Menu_LibraryClearSearches();
	view_dirty = SCE_TRUE;
	TouchList_Reset(&library_list);
}

// Cancelar deshace lo mas reciente primero, igual que el filtro del navegador
// de carpetas: con un termino en la lista que se ve lo quita y se queda en el
// mismo nivel, y sin termino sale del modo seleccion o, fuera de el, del grupo.
static void Menu_LibraryBack(void) {
	if (Menu_LibraryHasTerm())
		Menu_LibrarySetTerm("");
	else if (selecting)
		Menu_LibraryEndSelect();
	else
		Menu_LibraryLeave();
}

// Confirmar sobre la fila marcada: entra en un nombre o en una playlist, o
// reproduce una pista.
static void Menu_LibraryConfirm(void) {
	if (Menu_LibraryInPlaylist())
		Menu_LibraryPlayPlaylist();
	else if (Menu_LibraryShowsPlaylists())
		Menu_LibraryEnterPlaylist();
	else if (Menu_LibraryShowsNames())
		Menu_LibraryEnter();
	else
		Menu_LibraryPlaySelected();
}

// Tocar una fila es seleccionarla y confirmar; en el modo seleccion, marcarla o
// desmarcarla. `row` es la fila tocada este fotograma, o -1.
static SceBool Menu_HandleLibraryTap(Menu_LibraryState state, int row) {
	if (state != LIBRARY_STATE_READY || row < 0 || row >= Menu_LibraryBuild())
		return SCE_FALSE;

	selection = row;

	if (selecting) {
		header_focus = SCE_FALSE;
		Selection_Toggle(&marks, Menu_LibraryFull(row));
	}
	else {
		Menu_LibraryConfirm();
	}

	return SCE_TRUE;
}

// Mantener el dedo sobre una fila entra en el modo seleccion con ella marcada
// y como fila actual, y dentro del modo la marca o la desmarca. En la pestaña
// Playlists no hace nada: la lista pide el toque mantenido igual, para que
// levantar el dedo despues no entre en la playlist. `row` es la fila mantenida
// este fotograma, o -1.
static SceBool Menu_HandleLibraryHold(Menu_LibraryState state, int row) {
	if (state != LIBRARY_STATE_READY || row < 0 || row >= Menu_LibraryBuild())
		return SCE_FALSE;

	if (Menu_LibraryShowsPlaylists())
		return SCE_TRUE;

	if (!selecting)
		Menu_LibraryStartSelect();

	selection = row;
	header_focus = SCE_FALSE;
	Selection_Toggle(&marks, Menu_LibraryFull(row));
	return SCE_TRUE;
}

// Sin nada que reproducir, confirmar - o tocar la accion - hace lo unico que
// procede en cada uno de los otros tres estados.
static void Menu_LibraryEmptyAction(Menu_LibraryState state) {
	if (state == LIBRARY_STATE_NO_ROOT)
		Menu_LibraryPickRoot();
	else
		Menu_LibraryRescan();
}

// Las pestañas y la accion de un estado vacio se tocan como los botones que
// las accionan: al apoyar el dedo, como el riel y la caja de busqueda.
static SceBool Menu_HandleLibraryTouchControls(Menu_LibraryState state) {
	if (!Touch_CheckPressed())
		return SCE_FALSE;

	// En el encabezado del modo no hay pestañas: el aspa sale, como volver sin
	// busqueda, y la casilla Todo actua sobre lo visible si hay algo visible.
	if (state == LIBRARY_STATE_READY && selecting) {
		if (UI_TouchTarget(Menu_LibraryHeaderExitX(), 0, HEADER_EXIT, TOPBAR_H)) {
			Menu_LibraryEndSelect();
			return SCE_TRUE;
		}

		float ax = Menu_LibraryHeaderAllX();

		if (UI_TouchTarget(ax, 0, 960 - HEADER_RIGHT - ax, TOPBAR_H)) {
			Menu_LibraryBuild();
			Selection_ToggleAll(&marks, Menu_LibraryFilter());
			return SCE_TRUE;
		}

		return SCE_FALSE;
	}

	if (state == LIBRARY_STATE_READY) {
		for (int i = 0; i < VIEW_COUNT; i++) {
			float w = 0;
			float x = Menu_LibraryTabX(i, &w);

			if (UI_TouchTarget(x, 0, w, TABS_BOX_H)) {
				if (i != (int)view)
					Menu_LibrarySetView((Menu_LibraryTab)i);

				return SCE_TRUE;
			}
		}

		return SCE_FALSE;
	}

	const char *action = Lang_Get(empty_action[state]);
	float w = (float)UI_TextWidth(UI_FACE_MONO, UI_TS_LABEL, action);

	if (UI_TouchTarget(PLACEHOLDER_X, PLACEHOLDER_ACTION_Y, w, PLACEHOLDER_ACTION_H)) {
		Menu_LibraryEmptyAction(state);
		return SCE_TRUE;
	}

	return SCE_FALSE;
}

// Arma el menu para la fila marcada y lo abre con la marca en la primera
// opcion. Sin filas - una pestaña Playlists o una playlist vacias, o una
// busqueda sin resultados - solo quedan reescanear y elegir carpeta. En el modo
// seleccion tiene solo la accion sobre lo marcado, apagada si no hay nada.
static void Menu_LibraryOpenOptions(Menu_LibraryState state) {
	option_count = 0;
	options_disabled = SCE_FALSE;

	if (selecting) {
		options[option_count++] = Menu_LibraryInPlaylist() ? OPTION_REMOVE_FROM_PLAYLIST : OPTION_ADD_TO_PLAYLIST;
		options_disabled = (Selection_Count(&marks) == 0);
		options_open = SCE_TRUE;
		options_index = 0;
		return;
	}

	if (state == LIBRARY_STATE_READY && Menu_LibraryBuild() > 0) {
		if (Menu_LibraryInPlaylist()) {
			options[option_count++] = OPTION_REMOVE_FROM_PLAYLIST;
			options[option_count++] = OPTION_MOVE;
			options[option_count++] = OPTION_SELECT;
		}
		else if (Menu_LibraryShowsPlaylists()) {
			options[option_count++] = OPTION_RENAME;
			options[option_count++] = OPTION_DELETE;
		}
		else {
			options[option_count++] = OPTION_ADD_TO_PLAYLIST;
			options[option_count++] = OPTION_SELECT;
		}
	}

	options[option_count++] = OPTION_RESCAN;
	options[option_count++] = OPTION_CHOOSE_FOLDER;
	options_open = SCE_TRUE;
	options_index = 0;
}

static void Menu_HandleLibraryControls(Menu_LibraryState state) {
	int count = Menu_LibraryBuild();

	// L y R recorren las cuatro vistas. Cambiar de vista sale de cualquier grupo
	// en el que se estuviera.
	if (state == LIBRARY_STATE_READY) {
		if (pressed & SCE_CTRL_RTRIGGER)
			Menu_LibrarySetView((view + 1) % VIEW_COUNT);
		else if (pressed & SCE_CTRL_LTRIGGER)
			Menu_LibrarySetView((view + VIEW_COUNT - 1) % VIEW_COUNT);

		count = Menu_LibraryBuild();
	}

	if (state == LIBRARY_STATE_READY && count > 0) {
		int before = selection;
		Utils_ListStep(&selection, count, ROWS_PER_PAGE);

		if (pressed & SCE_CTRL_LEFT)
			selection = 0;
		else if (pressed & SCE_CTRL_RIGHT)
			selection = count - 1;

		// Un boton de direccion detiene el deslizamiento y la vista vuelve a la
		// seleccion, aunque el dedo la hubiera dejado fuera.
		if (selection != before || (pressed & (SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT | SCE_CTRL_RIGHT)))
			TouchList_Follow(&library_list);

		if (pressed & SCE_CTRL_ENTER)
			Menu_LibraryConfirm();
	}
	else if ((state != LIBRARY_STATE_READY) && (pressed & SCE_CTRL_ENTER))
		Menu_LibraryEmptyAction(state);

	// Fuera del bloque de la lista: una busqueda sin resultados deja la lista
	// vacia, y volver tiene que seguir quitandola.
	if ((state == LIBRARY_STATE_READY) && (pressed & SCE_CTRL_CANCEL))
		Menu_LibraryBack();

	// Triangulo busca, y solo donde hay en que buscar.
	if ((state == LIBRARY_STATE_READY) && (pressed & SCE_CTRL_TRIANGLE))
		Menu_LibraryPromptSearch();
	else if (pressed & SCE_CTRL_SQUARE)
		Menu_LibraryOpenOptions(state);
}

// Los botones del modo seleccion. Arriba y abajo recorren la casilla Todo y las
// filas, confirmar marca la fila o activa la casilla, volver quita la busqueda
// o sale del modo, Triangulo busca y Cuadrado abre las acciones. L, R y SELECT
// no hacen nada.
static void Menu_HandleLibrarySelect(Menu_LibraryState state) {
	int count = Menu_LibraryBuild();

	// Sin filas visibles la casilla no se alcanza: la posicion es una fila.
	if (count == 0)
		header_focus = SCE_FALSE;

	if (count > 0) {
		int before = header_focus ? 0 : selection + 1;
		int pos = Selection_Step(before, count, Utils_GetListStep(), ROWS_PER_PAGE);

		if (pressed & SCE_CTRL_LEFT)
			pos = 1;
		else if (pressed & SCE_CTRL_RIGHT)
			pos = count;

		header_focus = (pos == 0);

		if (pos > 0)
			selection = pos - 1;

		if (pos != before || (pressed & (SCE_CTRL_UP | SCE_CTRL_DOWN | SCE_CTRL_LEFT | SCE_CTRL_RIGHT)))
			TouchList_Follow(&library_list);

		if (pressed & SCE_CTRL_ENTER) {
			if (header_focus)
				Selection_ToggleAll(&marks, Menu_LibraryFilter());
			else
				Selection_Toggle(&marks, Menu_LibraryFull(selection));
		}
	}

	if (pressed & SCE_CTRL_CANCEL)
		Menu_LibraryBack();

	if (pressed & SCE_CTRL_TRIANGLE)
		Menu_LibraryPromptSearch();
	else if (pressed & SCE_CTRL_SQUARE)
		Menu_LibraryOpenOptions(state);
}

// --- añadir a una playlist -------------------------------------------------

static void Menu_LibraryFillEntry(Playlist_Entry *e, const Library_Track *t) {
	snprintf(e->path, sizeof(e->path), "%s", t->path);
	snprintf(e->title, sizeof(e->title), "%s", t->title);
	snprintf(e->artist, sizeof(e->artist), "%s", t->artist);
	snprintf(e->album, sizeof(e->album), "%s", t->album);
}

// Lo que se añade a una playlist, pedido de a una entrada por
// Playlist_AppendFrom: `n` entradas que da `next`.
typedef struct {
	int n;
	Playlist_Source next;
	void *ctx;
} Menu_LibraryAdding;

static int Menu_LibraryArrayEntry(int i, Playlist_Entry *out, void *ctx) {
	*out = ((const Playlist_Entry *)ctx)[i];
	return 1;
}

// Por donde va el recorrido de las marcas: la marca vigente y, en Artistas y
// Albumes, la pista del grupo vigente. La fuente empieza de nuevo al pedirle
// la entrada 0.
typedef struct {
	int mark;
	int track;
	int tracks;
} Menu_LibraryMarkCursor;

// Una cancion marcada por entrada, en el orden de la lista completa.
static int Menu_LibraryMarkedTrack(int i, Playlist_Entry *out, void *ctx) {
	Menu_LibraryMarkCursor *c = ctx;

	if (i == 0)
		c->mark = -1;

	c->mark = Selection_Next(&marks, c->mark + 1);

	const Library_Track *t = (c->mark >= 0) ? Library_ViewTrack(c->mark) : NULL;

	if (t == NULL)
		return 0;

	Menu_LibraryFillEntry(out, t);
	return 1;
}

// Las pistas de cada artista o album marcado, grupo por grupo en el orden de la
// lista y cada uno en el orden en que se ve al entrar en el. Cada grupo se
// construye sobre el arreglo de pistas, que la lista de nombres no usa.
static int Menu_LibraryMarkedGroupTrack(int i, Playlist_Entry *out, void *ctx) {
	Menu_LibraryMarkCursor *c = ctx;

	if (i == 0) {
		c->mark = -1;
		c->track = c->tracks = 0;
	}

	while (c->track >= c->tracks) {
		c->mark = Selection_Next(&marks, c->mark + 1);

		if (c->mark < 0)
			return 0;

		const char *name = Library_NameAt(c->mark);

		c->track = 0;
		c->tracks = (name != NULL) ? Library_BuildFieldTracks(Menu_LibraryField(), name, Library_NameIsUnknown(c->mark)) : 0;
	}

	const Library_Track *t = Library_ViewTrack(c->track++);

	if (t == NULL)
		return 0;

	Menu_LibraryFillEntry(out, t);
	return 1;
}

// Lo marcado en el modo seleccion. AppendFrom necesita saber cuantas antes de
// empezar, asi que en Artistas y Albumes una primera pasada suma los grupos y
// la fuente los vuelve a construir al entregarlos.
static void Menu_LibraryMarked(Menu_LibraryAdding *a, Menu_LibraryMarkCursor *c) {
	Menu_LibraryBuild();
	a->ctx = c;

	if (!Menu_LibraryShowsNames()) {
		a->n = Selection_Count(&marks);
		a->next = Menu_LibraryMarkedTrack;
		return;
	}

	a->n = 0;
	a->next = Menu_LibraryMarkedGroupTrack;

	for (int g = Selection_Next(&marks, 0); g >= 0; g = Selection_Next(&marks, g + 1)) {
		const char *name = Library_NameAt(g);

		if (name != NULL)
			a->n += Library_BuildFieldTracks(Menu_LibraryField(), name, Library_NameIsUnknown(g));
	}
}

// Lo que se añade desde la fila marcada, en `*out`, que el llamador libera: la
// cancion, o todas las pistas del artista o del album en el orden en que se ven
// al entrar en el. Devuelve cuantas, 0 si no hay nada o falta memoria.
static int Menu_LibraryCollect(Playlist_Entry **out) {
	*out = NULL;
	Menu_LibraryBuild();

	int full = Menu_LibraryFull(selection);
	int n = 1;
	const Library_Track *one = NULL;

	if (Menu_LibraryShowsNames()) {
		const char *name = Library_NameAt(full);

		if (name == NULL)
			return 0;

		// Construye la vista del grupo sobre el arreglo de pistas, que la lista
		// de nombres no usa: los nombres y sus caratulas siguen como estaban.
		n = Library_BuildFieldTracks(Menu_LibraryField(), name, Library_NameIsUnknown(full));
	}
	else if ((one = Library_ViewTrack(full)) == NULL) {
		return 0;
	}

	if (n <= 0)
		return 0;

	Playlist_Entry *entries = (Playlist_Entry *)calloc((size_t)n, sizeof(Playlist_Entry));

	if (entries == NULL)
		return 0;

	for (int i = 0; i < n; i++) {
		const Library_Track *t = (one != NULL) ? one : Library_ViewTrack(i);

		if (t == NULL) {
			n = i;
			break;
		}

		Menu_LibraryFillEntry(&entries[i], t);
	}

	*out = entries;
	return n;
}

// Si otra playlist que la del archivo `except` (NULL: ninguna) ya se llama asi.
static SceBool Menu_LibraryNameTaken(const char *name, const char *except) {
	Menu_LibraryLoadPlaylists();

	for (int i = 0; i < playlist_count; i++) {
		if ((except == NULL || strcmp(playlists[i].file, except) != 0) && Playlist_NamesEqual(playlists[i].name, name))
			return SCE_TRUE;
	}

	return SCE_FALSE;
}

static void Menu_LibraryNoticeAdded(const char *name, int added, int skipped, int full) {
	Menu_LibraryNoticef((added == 1) ? STR_NOTICE_ADDED_ONE : STR_NOTICE_ADDED_MANY, added, name);

	if (skipped > 0)
		Menu_LibraryNoticeAppend((skipped == 1) ? STR_NOTICE_SKIPPED_ONE : STR_NOTICE_SKIPPED_MANY, skipped);

	if (full)
		Menu_LibraryNoticeAppend(STR_NOTICE_PLAYLIST_FULL);
}

// Carga la playlist, le añade, la guarda y la suelta: no queda en RAM.
// SCE_TRUE si la playlist se escribio.
static SceBool Menu_LibraryAddToExisting(const char *file, const Menu_LibraryAdding *a) {
	Playlist pl;
	int added = 0, skipped = 0, full = 0;
	SceBool wrote = SCE_FALSE;

	if (PlaylistStore_Load(file, &pl) != PLAYLIST_OK) {
		Menu_LibraryNotice(STR_NOTICE_PLAYLIST_UNREADABLE);
		return SCE_FALSE;
	}

	if (Playlist_AppendFrom(&pl, a->n, a->next, a->ctx, &added, &skipped, &full) != PLAYLIST_OK || PlaylistStore_Save(file, &pl) < 0) {
		Menu_LibraryNotice(STR_NOTICE_PLAYLIST_SAVE_FAILED);
	}
	else {
		Menu_LibraryNoticeAdded(pl.name, added, skipped, full);
		wrote = SCE_TRUE;
	}

	Playlist_Free(&pl);
	return wrote;
}

// "Nueva playlist…": pide el nombre y crea la playlist con lo que se añade, en
// un solo paso. Descartar, un nombre vacio o uno repetido no crean nada.
// SCE_TRUE si la playlist se escribio.
static SceBool Menu_LibraryAddToNew(const Menu_LibraryAdding *a) {
	char name[PLAYLIST_NAME_MAX];

	if (SearchDialog_Prompt(Lang_Get(STR_PLAYLIST_NAME), "", name, sizeof(name),
		Menu_DrawLibraryContent, UI_SCREEN_LIBRARY) != SEARCH_DIALOG_CONFIRMED)
		return SCE_FALSE;

	if (!Playlist_NameValid(name))
		return SCE_FALSE;

	if (Menu_LibraryNameTaken(name, NULL)) {
		Menu_LibraryNoticef(STR_NOTICE_NAME_TAKEN, name);
		return SCE_FALSE;
	}

	Playlist pl;
	char file[PLAYLIST_FILE_MAX];
	int added = 0, skipped = 0, full = 0;
	SceBool wrote = SCE_FALSE;

	memset(&pl, 0, sizeof(pl));
	snprintf(pl.name, sizeof(pl.name), "%s", name);

	if (Playlist_AppendFrom(&pl, a->n, a->next, a->ctx, &added, &skipped, &full) != PLAYLIST_OK || PlaylistStore_Create(&pl, file) < 0) {
		Menu_LibraryNotice(STR_NOTICE_PLAYLIST_SAVE_FAILED);
	}
	else {
		Menu_LibraryNoticeAdded(pl.name, added, skipped, full);
		wrote = SCE_TRUE;
	}

	Playlist_Free(&pl);
	return wrote;
}

static void Menu_LibraryOpenPicker(void) {
	Menu_LibraryLoadPlaylists();
	picker_open = SCE_TRUE;
	picker_index = 0;
	picker_top = 0;
}

// Cierra el selector. Los resumenes que leyo se sueltan: son de la pestaña
// Playlists, que es la unica que los guarda.
static void Menu_LibraryClosePicker(void) {
	picker_open = SCE_FALSE;

	if (view != VIEW_PLAYLISTS)
		Menu_LibraryFreePlaylists();
}

// Añade lo marcado en el modo seleccion, o si no la fila marcada. Una accion
// que escribio la playlist sale del modo; cerrar el dialogo del nombre o uno
// vacio o repetido lo dejan con las mismas marcas.
static void Menu_LibraryRunPicker(int item) {
	Playlist_Entry *entries = NULL;
	Menu_LibraryAdding adding;
	Menu_LibraryMarkCursor cursor;

	if (selecting) {
		Menu_LibraryMarked(&adding, &cursor);
	}
	else {
		adding.n = Menu_LibraryCollect(&entries);
		adding.next = Menu_LibraryArrayEntry;
		adding.ctx = entries;
	}

	char file[PLAYLIST_FILE_MAX] = "";
	const PlaylistSummary *s = Menu_LibraryPlaylistAt(item - 1);

	// El archivo se copia antes de cerrar, que suelta los resumenes. El
	// dialogo del nombre se dibuja sobre la pantalla sin el selector.
	if (s != NULL)
		snprintf(file, sizeof(file), "%s", s->file);

	picker_open = SCE_FALSE;

	SceBool wrote = SCE_FALSE;

	if (adding.n > 0 && item == 0)
		wrote = Menu_LibraryAddToNew(&adding);
	else if (adding.n > 0 && file[0] != '\0')
		wrote = Menu_LibraryAddToExisting(file, &adding);

	free(entries);

	if (wrote) {
		Menu_LibraryReloadPlaylists();
		Menu_LibraryEndSelect();
	}

	Menu_LibraryClosePicker();
}

// Como el menu: arriba y abajo lo recorren, confirmar o tocar una opcion la
// elige, y volver o tocar fuera lo cierra sin añadir nada.
static void Menu_HandleLibraryPicker(void) {
	int shown = Menu_LibraryPickerShown();

	if (Touch_CheckPressed()) {
		for (int row = 0; row < shown; row++) {
			if (UI_TouchTarget(Menu_LibraryPickerX(), Menu_LibraryPickerRowY(row), PICKER_W, OPTIONS_ITEM_H)) {
				Menu_LibraryRunPicker(picker_top + row);
				return;
			}
		}

		Menu_LibraryClosePicker();
		return;
	}

	Utils_ListStep(&picker_index, Menu_LibraryPickerCount(), shown);

	if (picker_index < picker_top)
		picker_top = picker_index;
	else if (picker_index >= picker_top + shown)
		picker_top = picker_index - shown + 1;

	if (pressed & SCE_CTRL_ENTER)
		Menu_LibraryRunPicker(picker_index);
	else if (pressed & SCE_CTRL_CANCEL)
		Menu_LibraryClosePicker();
}

// --- editar la playlist abierta ---------------------------------------------

// Guarda la playlist abierta despues de cambiarla, y rehace los resumenes de la
// pestaña, donde ahora va primera. Si no se pudo guardar, la copia en RAM ya no
// es la del disco: se suelta y la lista se vuelve a leer del archivo.
static void Menu_LibrarySaveOpen(void) {
	if (PlaylistStore_Save(open_file, &open_playlist) < 0) {
		Menu_LibraryNotice(STR_NOTICE_PLAYLIST_SAVE_FAILED);
		Menu_LibraryFreeOpenPlaylist();
	}

	Menu_LibraryReloadPlaylists();
	view_dirty = SCE_TRUE;
}

// Sin confirmacion. Queda marcada la que ocupaba su lugar, o la anterior si
// era la ultima.
static void Menu_LibraryRemoveSelected(void) {
	int full = Menu_LibraryFull(selection);

	if (Menu_LibraryEntryAt(full) == NULL)
		return;

	Playlist_Remove(&open_playlist, full);
	Menu_LibrarySaveOpen();

	int count = Menu_LibraryBuild();

	if (selection >= count)
		selection = (count > 0) ? count - 1 : 0;

	TouchList_Follow(&library_list);
}

// Mover recorre la playlist entera, asi que una busqueda aplicada se quita
// antes: la cancion sigue marcada.
static void Menu_LibraryStartMove(void) {
	if (Menu_LibraryHasTerm())
		Menu_LibrarySetTerm("");

	int full = Menu_LibraryFull(selection);

	if (Menu_LibraryEntryAt(full) == NULL)
		return;

	moving = SCE_TRUE;
	move_origin = move_at = full;
}

// Arriba y abajo la desplazan un puesto por pulsacion, sin envolver por los
// extremos. Confirmar la deja ahi y guarda; volver la devuelve a su puesto.
static void Menu_HandleLibraryMove(void) {
	int target = move_at;

	if ((pressed & SCE_CTRL_UP) && move_at > 0)
		target = move_at - 1;
	else if ((pressed & SCE_CTRL_DOWN) && move_at < open_playlist.count - 1)
		target = move_at + 1;

	if (target != move_at && Playlist_Move(&open_playlist, move_at, target) == 0) {
		move_at = target;
		selection = move_at;
		TouchList_Follow(&library_list);
	}

	if (pressed & SCE_CTRL_ENTER) {
		moving = SCE_FALSE;

		if (move_at != move_origin)
			Menu_LibrarySaveOpen();
	}
	else if (pressed & SCE_CTRL_CANCEL) {
		Playlist_Move(&open_playlist, move_at, move_origin);
		selection = move_origin;
		moving = SCE_FALSE;
		TouchList_Follow(&library_list);
	}
}

// --- renombrar y borrar una playlist ---------------------------------------

// Marca en la pestaña la fila de la playlist del archivo `file`, o acota la
// marca si ya no esta.
static void Menu_LibrarySelectPlaylist(const char *file) {
	view_dirty = SCE_TRUE;

	int count = Menu_LibraryBuild();

	for (int v = 0; v < count; v++) {
		const PlaylistSummary *s = Menu_LibraryPlaylistAt(Menu_LibraryFull(v));

		if (file != NULL && s != NULL && strcmp(s->file, file) == 0) {
			selection = v;
			TouchList_Follow(&library_list);
			return;
		}
	}

	if (selection >= count)
		selection = (count > 0) ? count - 1 : 0;

	TouchList_Follow(&library_list);
}

// Con el nombre actual ya escrito. Descartar, un nombre vacio o el mismo dejan
// todo como estaba; uno que ya tiene otra playlist lo avisa.
static void Menu_LibraryRenameSelected(void) {
	const PlaylistSummary *s = Menu_LibraryPlaylistAt(Menu_LibraryFull(selection));
	char file[PLAYLIST_FILE_MAX], current[PLAYLIST_NAME_MAX], name[PLAYLIST_NAME_MAX];

	if (s == NULL)
		return;

	snprintf(file, sizeof(file), "%s", s->file);
	snprintf(current, sizeof(current), "%s", s->name);

	if (SearchDialog_Prompt(Lang_Get(STR_PLAYLIST_NAME), current, name, sizeof(name),
		Menu_DrawLibraryContent, UI_SCREEN_LIBRARY) != SEARCH_DIALOG_CONFIRMED)
		return;

	if (!Playlist_NameValid(name) || strcmp(name, current) == 0)
		return;

	if (Menu_LibraryNameTaken(name, file)) {
		Menu_LibraryNoticef(STR_NOTICE_NAME_TAKEN, name);
		return;
	}

	char new_file[PLAYLIST_FILE_MAX];

	if (PlaylistStore_Rename(file, name, new_file) < 0) {
		Menu_LibraryNotice(STR_NOTICE_PLAYLIST_SAVE_FAILED);
		return;
	}

	Menu_LibraryReloadPlaylists();
	Menu_LibrarySelectPlaylist(new_file);
}

static void Menu_LibraryAskDelete(void) {
	const PlaylistSummary *s = Menu_LibraryPlaylistAt(Menu_LibraryFull(selection));

	if (s == NULL)
		return;

	snprintf(confirm_file, sizeof(confirm_file), "%s", s->file);
	snprintf(confirm_name, sizeof(confirm_name), "%s", s->name);
	confirm_kind = CONFIRM_DELETE_PLAYLIST;
	confirm_index = CONFIRM_CANCEL;
	confirm_open = SCE_TRUE;
}

// "Quitar de la playlist" en el modo seleccion: cuantas y de cual.
static void Menu_LibraryAskRemoveMarked(void) {
	if (!open_loaded || Selection_Count(&marks) == 0)
		return;

	snprintf(confirm_name, sizeof(confirm_name), "%s", open_playlist.name);
	confirm_count = Selection_Count(&marks);
	confirm_kind = CONFIRM_REMOVE_MARKED;
	confirm_index = CONFIRM_CANCEL;
	confirm_open = SCE_TRUE;
}

static int Menu_LibraryMarkedDoomed(int index, void *ctx) {
	(void)ctx;
	return Selection_Has(&marks, index);
}

// Quita todas las marcadas, tambien las atenuadas y las que la busqueda oculta,
// y guarda una sola vez. Queda marcada la que ahora ocupa el lugar de la
// primera quitada, o la ultima que se ve si esa no se ve o ya no hay.
static void Menu_LibraryRemoveMarked(void) {
	char name[PLAYLIST_NAME_MAX];
	int first = -1;

	snprintf(name, sizeof(name), "%s", open_playlist.name);

	int removed = Playlist_RemoveIf(&open_playlist, Menu_LibraryMarkedDoomed, NULL, &first);

	Menu_LibraryEndSelect();

	if (removed == 0)
		return;

	// Si no se pudo guardar, la playlist abierta se solto y el aviso ya lo dice.
	Menu_LibrarySaveOpen();

	if (open_loaded)
		Menu_LibraryNoticef((removed == 1) ? STR_NOTICE_REMOVED_ONE : STR_NOTICE_REMOVED_MANY, removed, name);

	int count = Menu_LibraryBuild();
	int kept = ListFilter_ToVisible(Menu_LibraryFilter(), first);

	selection = (kept >= 0) ? kept : ((count > 0) ? count - 1 : 0);
	TouchList_Follow(&library_list);
}

// Borrar una playlist borra su archivo y nada mas: ni la musica que lista ni la
// cola que suena, que es una copia. Cancelar deja todo como estaba, tambien el
// modo seleccion con sus marcas.
static void Menu_LibraryRunConfirm(int item) {
	confirm_open = SCE_FALSE;

	if (item != CONFIRM_ACTION)
		return;

	if (confirm_kind == CONFIRM_REMOVE_MARKED) {
		Menu_LibraryRemoveMarked();
		return;
	}

	if (PlaylistStore_Delete(confirm_file) < 0)
		Menu_LibraryNotice(STR_NOTICE_PLAYLIST_SAVE_FAILED);

	Menu_LibraryReloadPlaylists();
	Menu_LibrarySelectPlaylist(NULL);
}

// Como el menu: cancelar, volver o tocar fuera la dejan intacta.
static void Menu_HandleLibraryConfirm(void) {
	if (Touch_CheckPressed()) {
		for (int i = 0; i < CONFIRM_ITEMS; i++) {
			if (UI_TouchTarget(Menu_LibraryConfirmX(), Menu_LibraryConfirmItemY(i), PICKER_W, OPTIONS_ITEM_H)) {
				Menu_LibraryRunConfirm(i);
				return;
			}
		}

		confirm_open = SCE_FALSE;
		return;
	}

	Utils_ListStep(&confirm_index, CONFIRM_ITEMS, CONFIRM_ITEMS);

	if (pressed & SCE_CTRL_ENTER)
		Menu_LibraryRunConfirm(confirm_index);
	else if (pressed & SCE_CTRL_CANCEL)
		confirm_open = SCE_FALSE;
}

// Reescanear y elegir carpeta ya quitan la busqueda y salen del grupo: elegir
// carpeta vuelve a Canciones, y reescanear a la misma pestaña desde arriba.
static void Menu_LibraryRunOption(int i) {
	// La accion apagada no hace nada, y el menu sigue abierto.
	if (options_disabled)
		return;

	options_open = SCE_FALSE;

	switch (options[i]) {
		case OPTION_ADD_TO_PLAYLIST:
			Menu_LibraryOpenPicker();
			break;
		case OPTION_REMOVE_FROM_PLAYLIST:
			if (selecting)
				Menu_LibraryAskRemoveMarked();
			else
				Menu_LibraryRemoveSelected();
			break;
		case OPTION_MOVE:
			Menu_LibraryStartMove();
			break;
		case OPTION_RENAME:
			Menu_LibraryRenameSelected();
			break;
		case OPTION_DELETE:
			Menu_LibraryAskDelete();
			break;
		case OPTION_SELECT:
			Menu_LibraryStartSelect();
			break;
		case OPTION_RESCAN:
			Menu_LibraryRescan();
			break;
		case OPTION_CHOOSE_FOLDER:
			Menu_LibraryPickRoot();
			break;
		default:
			break;
	}
}

// Con el menu abierto la lista no recibe nada: arriba y abajo lo recorren,
// confirmar o tocar una opcion la ejecuta, y volver o tocar fuera lo cierra.
static void Menu_HandleLibraryOptions(void) {
	if (Touch_CheckPressed()) {
		for (int i = 0; i < option_count; i++) {
			if (UI_TouchTarget(Menu_LibraryOptionsX(), Menu_LibraryOptionY(i), OPTIONS_W, OPTIONS_ITEM_H)) {
				Menu_LibraryRunOption(i);
				return;
			}
		}

		options_open = SCE_FALSE;
		return;
	}

	Utils_ListStep(&options_index, option_count, option_count);

	if (pressed & SCE_CTRL_ENTER)
		Menu_LibraryRunOption(options_index);
	else if (pressed & SCE_CTRL_CANCEL)
		options_open = SCE_FALSE;
}

// Lo que la pantalla suelta al irse. La pestaña, el nivel, la busqueda y la
// fila se quedan, y lo soltado se vuelve a leer al volver.
static UI_Screen Menu_LibraryExit(UI_Screen next) {
	// Irse a mitad de un movimiento lo cancela: la playlist se vuelve a leer
	// del archivo, que no se toco. Irse tambien sale del modo seleccion.
	moving = SCE_FALSE;
	Menu_LibraryEndSelect();
	Menu_LibraryFreePlaylists();
	Menu_LibraryFreeOpenPlaylist();
	return next;
}

UI_Screen Menu_DisplayLibrary(void) {
	vita2d_set_clear_color(UI_COLOR_BG);

	// Las playlists se soltaron al irse: la lista se rehace leyendolas, con la
	// misma busqueda, y la fila elegida se acota por si alguna desaparecio.
	if (view == VIEW_PLAYLISTS)
		view_dirty = SCE_TRUE;

	if (Menu_LibraryGetState() == LIBRARY_STATE_READY) {
		int count = Menu_LibraryBuild();

		if (selection >= count)
			selection = (count > 0) ? count - 1 : 0;
	}

	// Al volver - de Reproduciendo o por el riel - la fila elegida se ve, y no
	// sigue ningun dedo ni deslizamiento de la vez anterior. La lista de la
	// biblioteca, a diferencia de las otras, distingue el toque mantenido.
	library_list.hold_ms = GESTURE_HOLD_MS;
	TouchList_Reset(&library_list);
	TouchList_Follow(&library_list);

	while (SCE_TRUE) {
		Menu_LibraryState state = Menu_LibraryGetState();

		UI_GpuBeginFrame();
		vita2d_clear_screen();

		Menu_DrawLibraryContent();

		const char *play_hint = Lang_Get((state != LIBRARY_STATE_READY) ? STR_HINT_START
			: (Menu_LibraryRowsOpen() ? STR_HINT_OPEN : STR_HINT_PLAY));
		// Abrir no se anuncia cuando una busqueda no dejo nada sobre que actuar.
		if (state == LIBRARY_STATE_READY && Menu_LibraryBuild() == 0)
			play_hint = NULL;
		// La leyenda anuncia lo que el boton hace ahora mismo: con un termino en
		// la lista que se ve, volver lo quita; sin el, sale del grupo.
		const char *back_hint = (state == LIBRARY_STATE_READY && Menu_LibraryHasTerm()) ? Lang_Get(STR_HINT_CLEAR_FILTER)
			: (inside ? Lang_Get(STR_HINT_RETURN) : NULL);
		const char *search_hint = (state == LIBRARY_STATE_READY) ? Lang_Get(STR_HINT_SEARCH) : NULL;
		const NavRail_Hint hints[] = {
			{ { HINT_BTN_CONFIRM }, 0, play_hint },
			{ { HINT_BTN_CANCEL }, 0, back_hint },
			{ { HINT_BTN_L, HINT_BTN_R }, 0, Lang_Get(STR_HINT_VIEWS) },
			{ { HINT_BTN_TRIANGLE }, 0, search_hint },
			{ { HINT_BTN_SQUARE }, 0, Lang_Get(STR_HINT_OPTIONS) },
			{ { HINT_BTN_START }, 0, Lang_Get(STR_HINT_SCREEN_OFF) },
		};
		// Con el menu delante, la leyenda es la del menu.
		const NavRail_Hint option_hints[] = {
			{ { HINT_BTN_CONFIRM }, 0, Lang_Get(STR_HINT_SELECT) },
			{ { HINT_BTN_CANCEL }, 0, Lang_Get(STR_HINT_CANCEL) },
			{ { HINT_BTN_START }, 0, Lang_Get(STR_HINT_SCREEN_OFF) },
		};

		// Mientras se mueve una cancion, solo las acciones de ese modo.
		const NavRail_Hint move_hints[] = {
			{ { HINT_BTN_UP }, 0, Lang_Get(STR_HINT_MOVE) },
			{ { HINT_BTN_CONFIRM }, 0, Lang_Get(STR_HINT_PLACE) },
			{ { HINT_BTN_CANCEL }, 0, Lang_Get(STR_HINT_CANCEL) },
			{ { HINT_BTN_START }, 0, Lang_Get(STR_HINT_SCREEN_OFF) },
		};

		// En el modo seleccion, las acciones del modo y sin L ni R.
		const NavRail_Hint select_hints[] = {
			{ { HINT_BTN_CONFIRM }, 0, Lang_Get(STR_HINT_MARK) },
			{ { HINT_BTN_CANCEL }, 0, Lang_Get(Menu_LibraryHasTerm() ? STR_HINT_CLEAR_FILTER : STR_HINT_EXIT) },
			{ { HINT_BTN_TRIANGLE }, 0, Lang_Get(STR_HINT_SEARCH) },
			{ { HINT_BTN_SQUARE }, 0, Lang_Get(STR_HINT_ACTIONS) },
			{ { HINT_BTN_START }, 0, Lang_Get(STR_HINT_SCREEN_OFF) },
		};

		if (moving)
			NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, move_hints, 4);
		else if (options_open || picker_open || confirm_open)
			NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, option_hints, 3);
		else if (selecting)
			NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, select_hints, 5);
		else
			NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, hints, 6);

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_LIBRARY);
		UI_Debug_Draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();

		Utils_ReadControls();
		Touch_Update();
		UI_Debug_Update();
		UI_Theme_RenewFallbackIfNeeded();

		// La lista toma solo los toques que empiezan en ella: no con el menu
		// delante ni sobre el aviso, que la tapa en parte.
		SceBool on_notice = Menu_LibraryNoticeShown() && Touch_GetY() >= Menu_LibraryNoticeY();
		int tapped_row = TouchList_Update(&library_list, !options_open && !picker_open && !confirm_open && !moving && !on_notice && state == LIBRARY_STATE_READY, NULL);
		int held_row = TouchList_HeldRow(&library_list);

		if (notice_frames > 0)
			notice_frames--;

		// El menu se lleva todo, tambien el toque sobre el riel: tocar fuera
		// de el lo cierra y nada mas.
		if (options_open) {
			Menu_HandleLibraryOptions();
			continue;
		}

		if (picker_open) {
			Menu_HandleLibraryPicker();
			continue;
		}

		if (confirm_open) {
			Menu_HandleLibraryConfirm();
			continue;
		}

		// L y R, Triangulo, Cuadrado y la lista no hacen nada mientras se
		// mueve; el riel y el mini reproductor siguen.
		if (moving) {
			if (tapped == UI_SCREEN_FOLDERS || tapped == UI_SCREEN_SETTINGS)
				return Menu_LibraryExit(tapped);
			else if (tapped == UI_SCREEN_NOW_PLAYING && Audio_HasTrack())
				return Menu_LibraryExit(tapped);

			if (!MiniPlayer_HandleTouch())
				Menu_HandleLibraryMove();

			continue;
		}

		if (tapped == UI_SCREEN_FOLDERS || tapped == UI_SCREEN_SETTINGS)
			return Menu_LibraryExit(tapped);
		else if (tapped == UI_SCREEN_NOW_PLAYING && Audio_HasTrack())
			return Menu_LibraryExit(tapped);

		if (MiniPlayer_HandleTouch())
			continue;

		// Tocar la caja abre la misma busqueda que Triangulo.
		if (state == LIBRARY_STATE_READY && Touch_Position(Menu_LibrarySearchBoxX(), Menu_LibrarySearchBoxY(),
			Menu_LibrarySearchBoxX() + SEARCH_W, Menu_LibrarySearchBoxY() + SEARCH_H)) {
			Menu_LibraryPromptSearch();
			continue;
		}

		if (Menu_HandleLibraryTouchControls(state))
			continue;

		if (!Menu_HandleLibraryHold(state, held_row) && !Menu_HandleLibraryTap(state, tapped_row)) {
			if (selecting)
				Menu_HandleLibrarySelect(state);
			else
				Menu_HandleLibraryControls(state);
		}

		// Reproducir una pista no entra a Reproduciendo: lo pide.
		UI_Screen requested = NavRequest_Take();
		if (requested != UI_SCREEN_NONE)
			return Menu_LibraryExit(requested);

		// SELECT no lleva a Ajustes durante el modo seleccion.
		if ((pressed & SCE_CTRL_SELECT) && !selecting)
			return Menu_LibraryExit(UI_SCREEN_SETTINGS);
	}
}
