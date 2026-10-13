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

// Reescanear y elegir carpeta, reunidas en el menu de Cuadrado. Es un estado
// del mismo lazo, no un lazo propio: mientras esta abierto, la pantalla solo
// atiende al menu.
typedef enum {
	OPTION_RESCAN = 0,
	OPTION_CHOOSE_FOLDER,
	OPTION_COUNT
} Menu_LibraryOption;

static const LangString option_label[OPTION_COUNT] = { STR_MENU_RESCAN, STR_MENU_CHOOSE_FOLDER };

static SceBool options_open = SCE_FALSE;
static int options_index = 0;

// Declarada aqui porque elegir carpeta la usa y se define mas abajo, junto a
// las otras dos que mueven la vista.
static void Menu_LibrarySetView(Menu_LibraryTab next);
static SceBool root_missing = SCE_FALSE;
static int root_check_frames = 0;
// El aviso se guarda como id y se resuelve al dibujar, para que cambiar de
// idioma con uno en pantalla lo muestre ya traducido.
static LangString notice = STR_COUNT;
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

static int Menu_LibraryMatchPlaylist(int index, void *folded) {
	const PlaylistSummary *s = Menu_LibraryPlaylistAt(index);

	return s != NULL && TextFold_Contains(s->name, folded);
}

// Cuantas entradas tiene la lista completa ya construida del nivel vigente.
static int Menu_LibraryFullCount(void) {
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
	TouchList_Reset(&library_list);
	TouchList_Follow(&library_list);
}

static void Menu_LibraryClearSearches(void) {
	outer_search.term[0] = outer_search.folded[0] = '\0';
	group_search.term[0] = group_search.folded[0] = '\0';
}

static void Menu_LibraryNotice(LangString text) {
	notice = text;
	notice_frames = NOTICE_FRAMES;
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

// La caja de busqueda, a la izquierda de las pestañas. La comparten el
// dibujado y el toque.
static float Menu_LibrarySearchBoxX(void) { return Menu_LibraryTabsLeft() - SEARCH_TAB_GAP - SEARCH_W; }
static float Menu_LibrarySearchBoxY(void) { return (TOPBAR_H - SEARCH_H) / 2.0f; }

static void Menu_DrawLibraryTopBar(void) {
	vita2d_draw_rectangle(CONTENT_X, 0, 960 - CONTENT_X, TOPBAR_H, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(CONTENT_X, TOPBAR_H - 1, 960 - CONTENT_X, 1, UI_COLOR_HAIRLINE);

	float x = CONTENT_X + 22;
	float sx = Menu_LibrarySearchBoxX(), sy = Menu_LibrarySearchBoxY();
	// El titulo y la ruta se recortan antes de la caja en vez de pasarle por
	// debajo: dentro de un album el titulo es su nombre, y puede ser largo.
	float text_w = sx - 16 - x;

	// Dentro de un artista o un album manda su nombre, porque es lo que el usuario
	// acaba de elegir; la pestaña sigue marcada al lado.
	const char *heading = inside ? (inside_unknown ? Lang_Get(STR_UNKNOWN) : inside_name) : Lang_Get(STR_LIBRARY_TITLE);

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
	if (Menu_LibraryGetState() == LIBRARY_STATE_READY) {
		SceBool has_term = Menu_LibraryHasTerm();

		UI_DrawPill(sx, sy, SEARCH_W, SEARCH_H, UI_COLOR_SURFACE);
		UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, sx + 14, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, sy, SEARCH_H),
			SEARCH_W - 28, has_term ? UI_COLOR_TEXT_PRIMARY : UI_COLOR_TEXT_MUTED,
			has_term ? Menu_LibrarySearchNow()->term : Lang_Get(STR_SEARCH_LIBRARY));
	}
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
	if (i == selection)
		UI_DrawRowHighlight(CONTENT_X, y, 960 - CONTENT_X, ROW_H);
	else if (i == TouchList_PressedRow(&library_list))
		UI_DrawRowPressHighlight(CONTENT_X, y, 960 - CONTENT_X, ROW_H);

	float art_x = CONTENT_X + 22, art_y = y + (ROW_H - ROW_ART) / 2.0f;

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

	// Sin playlists, la pestaña dice como se crea una.
	if (count == 0 && Menu_LibraryShowsPlaylists()) {
		Menu_DrawLibraryPlaceholder(Lang_Get(STR_PLAYLISTS_EMPTY), Lang_Get(STR_PLAYLISTS_EMPTY_HOW));
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

		if (Menu_LibraryShowsPlaylists()) {
			const PlaylistSummary *s = Menu_LibraryPlaylistAt(full);

			if (s == NULL)
				break;

			char sub[48];
			snprintf(sub, sizeof(sub), Lang_Get((s->count == 1) ? STR_TRACKS_ONE : STR_TRACKS_MANY), s->count);
			Menu_DrawLibraryRow(i, y, s->name, sub, NULL, NULL, SCE_FALSE);
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
	return notice_frames > 0 && notice < STR_COUNT;
}

static void Menu_DrawLibraryNotice(void) {
	if (!Menu_LibraryNoticeShown())
		return;

	float h = NOTICE_H, y = Menu_LibraryNoticeY();

	UI_DrawRoundedRect(CONTENT_X + 16, y, 960 - CONTENT_X - 32, h, 12, UI_COLOR_SURFACE_2);
	UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, CONTENT_X + 32, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, y, h),
		960 - CONTENT_X - 64, UI_COLOR_TEXT_PRIMARY, Lang_Get(notice));
}

static float Menu_LibraryOptionsH(void) { return OPTION_COUNT * OPTIONS_ITEM_H + 2 * OPTIONS_PAD; }
static float Menu_LibraryOptionsX(void) { return 960 - 16 - OPTIONS_W; }
static float Menu_LibraryOptionsY(void) { return MiniPlayer_Top() - 8 - Menu_LibraryOptionsH(); }

static float Menu_LibraryOptionY(int i) { return Menu_LibraryOptionsY() + OPTIONS_PAD + i * OPTIONS_ITEM_H; }

static void Menu_DrawLibraryOptions(void) {
	if (!options_open)
		return;

	float x = Menu_LibraryOptionsX();

	UI_DrawRoundedRect(x, Menu_LibraryOptionsY(), OPTIONS_W, Menu_LibraryOptionsH(), 14, UI_COLOR_SURFACE_2);

	for (int i = 0; i < OPTION_COUNT; i++) {
		float y = Menu_LibraryOptionY(i);

		if (i == options_index)
			UI_DrawRowHighlight(x + OPTIONS_PAD, y, OPTIONS_W - 2 * OPTIONS_PAD, OPTIONS_ITEM_H);

		UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x + OPTIONS_PAD + 16, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, OPTIONS_ITEM_H),
			OPTIONS_W - 2 * OPTIONS_PAD - 32, UI_COLOR_TEXT_PRIMARY, Lang_Get(option_label[i]));
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

static void Menu_LibraryEnter(void) {
	int full = Menu_LibraryFull(selection);
	const char *name = Library_NameAt(full);

	if (name == NULL)
		return;

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
// `outer_selection` sigue valiendo porque ese termino no cambio.
static SceBool Menu_LibraryLeave(void) {
	if (!inside)
		return SCE_FALSE;

	inside = SCE_FALSE;
	group_search.term[0] = group_search.folded[0] = '\0';
	inside_unknown = SCE_FALSE;
	inside_name[0] = '\0';
	selection = outer_selection;
	view_dirty = SCE_TRUE;
	TouchList_Reset(&library_list);
	TouchList_Follow(&library_list);
	return SCE_TRUE;
}

static void Menu_LibrarySetView(Menu_LibraryTab next) {
	// Los resumenes son de la pestaña: se sueltan al dejarla, y al volver a
	// ella se leen de nuevo.
	Menu_LibraryFreePlaylists();
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
// mismo nivel, y sin termino sale del grupo.
static void Menu_LibraryBack(void) {
	if (Menu_LibraryHasTerm())
		Menu_LibrarySetTerm("");
	else
		Menu_LibraryLeave();
}

// Confirmar sobre la fila marcada: entra en un nombre o en una playlist, o
// reproduce una pista.
static void Menu_LibraryConfirm(void) {
	if (Menu_LibraryShowsPlaylists())
		return;

	if (Menu_LibraryShowsNames())
		Menu_LibraryEnter();
	else
		Menu_LibraryPlaySelected();
}

// Tocar una fila es seleccionarla y confirmar. `row` es la fila tocada este
// fotograma, o -1.
static SceBool Menu_HandleLibraryTap(Menu_LibraryState state, int row) {
	if (state != LIBRARY_STATE_READY || row < 0 || row >= Menu_LibraryBuild())
		return SCE_FALSE;

	selection = row;
	Menu_LibraryConfirm();
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
	else if (pressed & SCE_CTRL_SQUARE) {
		options_open = SCE_TRUE;
		options_index = 0;
	}
}

// Las dos acciones ya quitan la busqueda y salen del grupo: elegir carpeta
// vuelve a Canciones, y reescanear a la misma pestaña desde arriba.
static void Menu_LibraryRunOption(int option) {
	options_open = SCE_FALSE;

	if (option == OPTION_RESCAN)
		Menu_LibraryRescan();
	else if (option == OPTION_CHOOSE_FOLDER)
		Menu_LibraryPickRoot();
}

// Con el menu abierto la lista no recibe nada: arriba y abajo lo recorren,
// confirmar o tocar una opcion la ejecuta, y volver o tocar fuera lo cierra.
static void Menu_HandleLibraryOptions(void) {
	if (Touch_CheckPressed()) {
		for (int i = 0; i < OPTION_COUNT; i++) {
			if (UI_TouchTarget(Menu_LibraryOptionsX(), Menu_LibraryOptionY(i), OPTIONS_W, OPTIONS_ITEM_H)) {
				Menu_LibraryRunOption(i);
				return;
			}
		}

		options_open = SCE_FALSE;
		return;
	}

	Utils_ListStep(&options_index, OPTION_COUNT, OPTION_COUNT);

	if (pressed & SCE_CTRL_ENTER)
		Menu_LibraryRunOption(options_index);
	else if (pressed & SCE_CTRL_CANCEL)
		options_open = SCE_FALSE;
}

// Lo que la pantalla suelta al irse. La pestaña, el nivel, la busqueda y la
// fila se quedan, y lo soltado se vuelve a leer al volver.
static UI_Screen Menu_LibraryExit(UI_Screen next) {
	Menu_LibraryFreePlaylists();
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
	// sigue ningun dedo ni deslizamiento de la vez anterior.
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

		if (options_open)
			NavRail_DrawHints(544 - UI_HINT_BAR_HEIGHT, option_hints, 3);
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
		int tapped_row = TouchList_Update(&library_list, !options_open && !on_notice && state == LIBRARY_STATE_READY, NULL);

		if (notice_frames > 0)
			notice_frames--;

		// El menu se lleva todo, tambien el toque sobre el riel: tocar fuera
		// de el lo cierra y nada mas.
		if (options_open) {
			Menu_HandleLibraryOptions();
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

		if (!Menu_HandleLibraryTap(state, tapped_row))
			Menu_HandleLibraryControls(state);

		// Reproducir una pista no entra a Reproduciendo: lo pide.
		UI_Screen requested = NavRequest_Take();
		if (requested != UI_SCREEN_NONE)
			return Menu_LibraryExit(requested);

		if (pressed & SCE_CTRL_SELECT)
			return Menu_LibraryExit(UI_SCREEN_SETTINGS);
	}
}
