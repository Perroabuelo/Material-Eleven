#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "common.h"
#include "cover.h"
#include "fs.h"
#include "lang.h"
#include "library.h"
#include "menu_audioplayer.h"
#include "menu_displayfiles.h"
#include "menu_library.h"
#include "menu_settings.h"
#include "mini_player.h"
#include "nav_rail.h"
#include "queue.h"
#include "status_bar.h"
#include "touch.h"
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

// Cuantos fotogramas se queda un aviso delante. A 60 fps son unos cuatro
// segundos: suficiente para leerlo, y se va solo sin pedir confirmacion.
#define NOTICE_FRAMES 240

// Cada cuantos fotogramas se vuelve a preguntar si la carpeta sigue ahi.
// A 60 fps, una vez por segundo.
#define ROOT_CHECK_PERIOD 60

// Las cuatro formas de recorrer la coleccion, y la etiqueta de cada una.
typedef enum {
	VIEW_SONGS = 0,
	VIEW_ARTISTS,
	VIEW_ALBUMS,
	VIEW_RECENT,
	VIEW_COUNT
} Menu_LibraryTab;

static const LangString view_label[VIEW_COUNT] = { STR_VIEW_SONGS, STR_VIEW_ARTISTS, STR_VIEW_ALBUMS, STR_VIEW_RECENT };

// La etiqueta del cubo de las pistas sin ese campo (STR_UNKNOWN). La pone la
// vista, no el indice: asi un artista que de verdad se llame asi no se mezcla con
// el cubo, y cambiar de idioma no obliga a reescanear.

static Menu_LibraryTab view = VIEW_SONGS;
static int selection = 0;

// Dentro de un artista o de un album. El nombre vacio con `inside_unknown`
// puesto es el cubo.
static SceBool inside = SCE_FALSE;
static SceBool inside_unknown = SCE_FALSE;
static char inside_name[LIBRARY_TAG_MAX] = "";
static int outer_selection = 0;

// Construir una vista es un qsort sobre miles de enteros. Se rehace cuando algo
// la invalida - cambiar de pestaña, entrar, salir, reescanear - y no en cada
// fotograma, que es lo que costaria dibujarla y leer el pad.
static SceBool view_dirty = SCE_TRUE;

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

static Library_Field Menu_LibraryField(void) {
	return (view == VIEW_ALBUMS) ? LIBRARY_FIELD_ALBUM : LIBRARY_FIELD_ARTIST;
}

static int Menu_LibraryBuilt(void) {
	return Menu_LibraryShowsNames() ? Library_NameCount() : Library_ViewCount();
}

static int Menu_LibraryBuild(void) {
	if (!view_dirty)
		return Menu_LibraryBuilt();

	view_dirty = SCE_FALSE;

	if (Menu_LibraryShowsNames())
		return Library_BuildFieldNames(Menu_LibraryField());

	if (inside)
		return Library_BuildFieldTracks(Menu_LibraryField(), inside_name, inside_unknown);

	if (view == VIEW_RECENT)
		return Library_BuildRecent();

	return Library_BuildSongs();
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

static void Menu_DrawLibraryTopBar(void) {
	vita2d_draw_rectangle(CONTENT_X, 0, 960 - CONTENT_X, TOPBAR_H, UI_COLOR_BG_ELEVATED);
	vita2d_draw_rectangle(CONTENT_X, TOPBAR_H - 1, 960 - CONTENT_X, 1, UI_COLOR_HAIRLINE);

	float x = CONTENT_X + 22;

	// Dentro de un artista o un album manda su nombre, porque es lo que el usuario
	// acaba de elegir; la pestaña sigue marcada al lado.
	const char *heading = inside ? (inside_unknown ? Lang_Get(STR_UNKNOWN) : inside_name) : Lang_Get(STR_LIBRARY_TITLE);

	UI_DrawText(UI_FACE_UI, UI_TS_TITLE, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_TITLE, 0, TOPBAR_H - 18),
		UI_COLOR_TEXT_PRIMARY, heading);

	// Las cuatro pestañas, con la vigente marcada. Se dibujan tambien dentro de un
	// grupo, para que se vea de donde se entro.
	float tab_x = 960 - 22;

	for (int i = VIEW_COUNT - 1; i >= 0; i--) {
		const char *label = Lang_Get(view_label[i]);
		float w = (float)UI_TextWidth(UI_FACE_MONO, UI_TS_BADGE, label);
		tab_x -= w;

		UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, tab_x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, 0, TOPBAR_H - 18),
			(i == view) ? ui_color_accent : UI_COLOR_TEXT_MUTED, label);

		tab_x -= 18.0f;
	}

	if (Library_HasRoot()) {
		UI_DrawTextClipped(UI_FACE_MONO, UI_TS_BADGE, x,
			UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, TOPBAR_H - 26, 20), 960 - x - 240,
			UI_COLOR_TEXT_TERTIARY, Library_GetRoot());
	}
}

// Un estado vacio que dice cual de los cuatro es y que hacer, en vez de una
// lista vacia sin explicacion.
static void Menu_DrawLibraryPlaceholder(const char *line, const char *action) {
	float x = CONTENT_X + 22, y = 180.0f;

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y, 26),
		960 - x - 22, UI_COLOR_TEXT_SECONDARY, line);

	if (action != NULL) {
		UI_DrawText(UI_FACE_MONO, UI_TS_LABEL, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, y + 34, 24),
			UI_COLOR_TEXT_TERTIARY, action);
	}
}

// Una fila: la linea de arriba es lo que la vista lista, y la de abajo lo que
// ayuda a distinguirlo de sus vecinas.
// `ext` es la extension del archivo en las filas que son una cancion, y NULL en
// las de artista o album, que no son un archivo y no llevan badge de formato.
static void Menu_DrawLibraryRow(int i, float y, const char *primary, const char *secondary,
		const char *album, const char *path, const char *ext, int *budget) {
	if (i == selection)
		UI_DrawRowHighlight(CONTENT_X, y, 960 - CONTENT_X, ROW_H);

	float art_x = CONTENT_X + 22, art_y = y + (ROW_H - ROW_ART) / 2.0f;
	vita2d_texture *art = (album != NULL || path != NULL) ? Cover_Get(album, path, budget) : NULL;

	if (art != NULL) {
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
	const char *badge_label = NULL; unsigned int badge_color = 0, badge_wash = 0;
	SceBool has_badge = (ext != NULL) && UI_GetFormatBadge(ext, &badge_label, &badge_color, &badge_wash);
	float badge_x = 0;

	if (has_badge) {
		badge_x = 960 - ROW_RIGHT_PAD - UI_BadgeWidth(UI_TS_BADGE, badge_label);
		text_w = badge_x - ROW_BADGE_GAP - x;
	}

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, y + 6, 24),
		text_w, UI_COLOR_TEXT_PRIMARY, primary);

	if (secondary != NULL) {
		UI_DrawTextClipped(UI_FACE_MONO, UI_TS_BADGE, x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, y + 34, 20),
			text_w, UI_COLOR_TEXT_TERTIARY, secondary);
	}

	if (has_badge)
		UI_DrawBadge(badge_x, y + (ROW_H - 30) / 2, UI_TS_BADGE, badge_label, badge_wash, badge_color);
}

static void Menu_DrawLibraryList(void) {
	int count = Menu_LibraryBuild();
	int first = selection - (selection % ROWS_PER_PAGE);
	float y = LIST_TOP;
	int budget = COVER_BUDGET_PER_FRAME;

	for (int i = first; i < first + ROWS_PER_PAGE && i < count; i++) {
		if (Menu_LibraryShowsNames()) {
			char sub[48];
			int n = Library_NameTrackCount(i);

			snprintf(sub, sizeof(sub), Lang_Get((n == 1) ? STR_TRACKS_ONE : STR_TRACKS_MANY), n);

			// En la vista de albumes la fila ES un album, asi que su caratula es la
			// del cubo. En la de artistas no hay una sola imagen que la represente.
			const char *art_album = (view == VIEW_ALBUMS && !Library_NameIsUnknown(i)) ? Library_NameAt(i) : NULL;

			Menu_DrawLibraryRow(i, y, Library_NameIsUnknown(i) ? Lang_Get(STR_UNKNOWN) : Library_NameAt(i), sub,
				art_album, NULL, NULL, &budget);
		}
		else {
			const Library_Track *track = Library_ViewTrack(i);

			if (track == NULL)
				break;

			const char *artist = (track->artist[0] != '\0') ? track->artist : Lang_Get(STR_UNKNOWN);
			Menu_DrawLibraryRow(i, y, Menu_LibraryTrackTitle(track), artist, track->album, track->path, track->ext, &budget);
		}

		y += ROW_H;
	}

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
		UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, LIST_TOP + ROWS_PER_PAGE * ROW_H + 4, 22),
		UI_COLOR_TEXT_MUTED, counter);
}

static void Menu_DrawLibraryNotice(void) {
	if (notice_frames <= 0 || notice >= STR_COUNT)
		return;

	float h = 44.0f, y = MiniPlayer_Top() - h - 8;

	UI_DrawRoundedRect(CONTENT_X + 16, y, 960 - CONTENT_X - 32, h, 12, UI_COLOR_SURFACE_2);
	UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, CONTENT_X + 32, UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, y, h),
		960 - CONTENT_X - 64, UI_COLOR_TEXT_PRIMARY, Lang_Get(notice));
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

	selection = 0;

	if (Library_RunScan() && Library_RunTagPass())
		Library_RunCoverPass();

	if (Library_IsTruncated())
		Menu_LibraryNotice(STR_NOTICE_TRUNCATED);
}

// El productor de biblioteca: vuelca la vista vigente en la cola, en el orden
// en que se ve, y reproduce desde ahi. Siguiente y anterior recorren esa cola y
// no la carpeta en la que este el archivo.
static void Menu_LibraryPlaySelected(void) {
	// La cola es esta vista, en el orden en que se ve. Por eso se reconstruye
	// aqui: es lo que hace que reproducir desde un album encadene el album y no
	// la carpeta en la que este cada archivo.
	Menu_LibraryBuild();

	const Library_Track *track = Library_ViewTrack(selection);

	if (track == NULL)
		return;

	// El indice es una cache de lo que habia en disco, y el disco pudo cambiar
	// desde el ultimo escaneo. Comprobarlo aqui es barato; validar el indice
	// entero al arrancar seria un escaneo.
	if (!FS_FileExists(track->path)) {
		Menu_LibraryNotice(STR_NOTICE_FILE_GONE);
		return;
	}

	Queue_Clear();

	for (int i = 0; i < Library_ViewCount(); i++) {
		const Library_Track *t = Library_ViewTrack(i);

		if (t == NULL)
			break;

		if (!Queue_Add(t->path, Menu_LibraryTrackTitle(t)))
			break;
	}

	// La biblioteca indexa por extension sin abrir nada, asi que lista tambien
	// lo que no se puede decodificar. Decirlo es mejor que no hacer nada.
	if (!Menu_PlayQueued(track->path))
		Menu_LibraryNotice(STR_NOTICE_UNREADABLE);
}

// ---------------------------------------------------------------------------

static void Menu_LibraryEnter(void) {
	const char *name = Library_NameAt(selection);

	if (name == NULL)
		return;

	inside_unknown = Library_NameIsUnknown(selection);
	snprintf(inside_name, sizeof(inside_name), "%s", name);
	outer_selection = selection;
	inside = SCE_TRUE;
	selection = 0;
	view_dirty = SCE_TRUE;
}

// SCE_TRUE si habia de donde salir. Cancelar deshace lo mas reciente primero,
// igual que hace el filtro en el navegador de carpetas.
static SceBool Menu_LibraryLeave(void) {
	if (!inside)
		return SCE_FALSE;

	inside = SCE_FALSE;
	inside_unknown = SCE_FALSE;
	inside_name[0] = '\0';
	selection = outer_selection;
	view_dirty = SCE_TRUE;
	return SCE_TRUE;
}

static void Menu_LibrarySetView(Menu_LibraryTab next) {
	view = next;
	inside = SCE_FALSE;
	inside_unknown = SCE_FALSE;
	inside_name[0] = '\0';
	selection = 0;
	outer_selection = 0;
	view_dirty = SCE_TRUE;
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
		if (pressed & SCE_CTRL_UP)
			selection--;
		else if (pressed & SCE_CTRL_DOWN)
			selection++;

		Utils_SetMax(&selection, 0, count - 1);
		Utils_SetMin(&selection, count - 1, 0);

		if (pressed & SCE_CTRL_LEFT)
			selection = 0;
		else if (pressed & SCE_CTRL_RIGHT)
			selection = count - 1;

		if (pressed & SCE_CTRL_ENTER) {
			// Sobre un nombre, confirmar entra; sobre una pista, reproduce.
			if (Menu_LibraryShowsNames())
				Menu_LibraryEnter();
			else
				Menu_LibraryPlaySelected();
		}

		if (pressed & SCE_CTRL_CANCEL)
			Menu_LibraryLeave();
	}
	else if (pressed & SCE_CTRL_ENTER) {
		// Sin nada que reproducir, confirmar hace lo unico que procede en cada
		// uno de los otros tres estados.
		if (state == LIBRARY_STATE_NO_ROOT)
			Menu_LibraryPickRoot();
		else
			Menu_LibraryRescan();
	}

	if (pressed & SCE_CTRL_TRIANGLE)
		Menu_LibraryRescan();
	else if (pressed & SCE_CTRL_SQUARE)
		Menu_LibraryPickRoot();
}

void Menu_DisplayLibrary(void) {
	vita2d_set_clear_color(UI_COLOR_BG);

	while (SCE_TRUE) {
		Menu_LibraryState state = Menu_LibraryGetState();

		vita2d_start_drawing();
		vita2d_clear_screen();

		StatusBar_Display();
		Menu_DrawLibraryTopBar();

		// La comprobacion es una llamada al sistema de archivos, asi que se
		// hace cada tantos fotogramas y no en todos.
		if ((root_check_frames--) <= 0) {
			root_check_frames = ROOT_CHECK_PERIOD;
			root_missing = Library_HasRoot() && !Library_RootAvailable();
		}

		if (root_missing) {
			UI_DrawTextClipped(UI_FACE_UI, UI_TS_LABEL, CONTENT_X + 22,
				UI_TextBaselineY(UI_FACE_UI, UI_TS_LABEL, TOPBAR_H + 10, 24), 960 - CONTENT_X - 44,
				UI_COLOR_TEXT_SECONDARY, Lang_Get(STR_ROOT_MISSING));
		}

		switch (state) {
			case LIBRARY_STATE_NO_ROOT:
				Menu_DrawLibraryPlaceholder(Lang_Get(STR_EMPTY_NO_ROOT), Lang_Get(STR_ACTION_CHOOSE_FOLDER));
				break;
			case LIBRARY_STATE_UNBUILT:
				Menu_DrawLibraryPlaceholder(Lang_Get(STR_EMPTY_UNBUILT), Lang_Get(STR_ACTION_SCAN));
				break;
			case LIBRARY_STATE_EMPTY:
				Menu_DrawLibraryPlaceholder(Lang_Get(STR_EMPTY_NO_TRACKS), Lang_Get(STR_ACTION_CHOOSE_OTHER));
				break;
			case LIBRARY_STATE_READY:
				Menu_DrawLibraryList();
				break;
		}

		MiniPlayer_Draw();
		Menu_DrawLibraryNotice();

		const char *play_hint = Lang_Get((state != LIBRARY_STATE_READY) ? STR_HINT_START
			: (Menu_LibraryShowsNames() ? STR_HINT_OPEN : STR_HINT_PLAY));
		// La leyenda anuncia lo que el boton hace ahora mismo: retomar etiquetas
		// solo aparece cuando queda alguna por leer.
		const char *back_hint = inside ? Lang_Get(STR_HINT_RETURN) : NULL;
		const char *hints[] = { play_hint, back_hint, Lang_Get(STR_HINT_VIEWS), Lang_Get(STR_HINT_RESCAN), Lang_Get(STR_HINT_FOLDER) };
		NavRail_DrawHintBar(544 - UI_HINT_BAR_HEIGHT, hints, 5);

		UI_Screen tapped = NavRail_DrawAndHitTest(UI_SCREEN_LIBRARY);
		UI_Debug_Draw();

		vita2d_end_drawing();
		vita2d_swap_buffers();

		Utils_ReadControls();
		Touch_Update();
		UI_Debug_Update();
		UI_Theme_RenewFallbackIfNeeded();

		if (notice_frames > 0)
			notice_frames--;

		if (tapped == UI_SCREEN_FOLDERS) {
			Touch_Reset();
			Menu_DisplayFiles();
			return;
		}
		else if (tapped == UI_SCREEN_SETTINGS) {
			Touch_Reset();
			Menu_DisplaySettings();
			return;
		}
		else if (tapped == UI_SCREEN_NOW_PLAYING && Audio_HasTrack()) {
			Touch_Reset();
			Menu_ShowNowPlaying();
			return;
		}

		if (MiniPlayer_HandleTouch())
			continue;

		Menu_HandleLibraryControls(state);

		if (pressed & SCE_CTRL_SELECT) {
			Menu_DisplaySettings();
			return;
		}

		if (pressed & SCE_CTRL_START)
			break;
	}
}
