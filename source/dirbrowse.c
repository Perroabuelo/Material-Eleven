#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_audioplayer.h"
#include "ui_theme.h"
#include "utils.h"

File *files = NULL;
static char filter_query[64] = "";

static void Dirbrowse_RecursiveFree(File *node) {
	if (node == NULL) // End of list
		return;

	Dirbrowse_RecursiveFree(node->next); // Nest further
	free(node); // Free memory
}

static void Dirbrowse_SaveLastDirectory(void) {
	char *buf = malloc(256);
	int len = snprintf(buf, 256, "%s\n", cwd);
	FS_WriteFile("ux0:data/elevenmpv/lastdir.txt", buf, len);
	free(buf);
}

static int cmpstringp(const void *p1, const void *p2) {
	SceIoDirent *entryA = (SceIoDirent *)p1;
	SceIoDirent *entryB = (SceIoDirent *)p2;

	if ((SCE_S_ISDIR(entryA->d_stat.st_mode)) && !(SCE_S_ISDIR(entryB->d_stat.st_mode)))
		return -1;
	else if (!(SCE_S_ISDIR(entryA->d_stat.st_mode)) && (SCE_S_ISDIR(entryB->d_stat.st_mode)))
		return 1;
	else {
		if (config.sort == 0) // Sort alphabetically (ascending - A to Z)
			return strcasecmp(entryA->d_name, entryB->d_name);
		else if (config.sort == 1) // Sort alphabetically (descending - Z to A)
			return strcasecmp(entryB->d_name, entryA->d_name);
		else if (config.sort == 2) // Sort by file size (largest first)
			return entryA->d_stat.st_size > entryB->d_stat.st_size ? -1 : entryA->d_stat.st_size < entryB->d_stat.st_size ? 1 : 0;
		else if (config.sort == 3) // Sort by file size (smallest first)
			return entryB->d_stat.st_size > entryA->d_stat.st_size ? -1 : entryB->d_stat.st_size < entryA->d_stat.st_size ? 1 : 0;
	}

	return 0;
}

int Dirbrowse_PopulateFiles(SceBool refresh) {
	SceUID dir = 0;

	// Between frames by construction: every caller reaches here from the input
	// handling that runs after vita2d_end_drawing and vita2d_swap_buffers.
	UI_Theme_RenewFallbackIfNeeded();

	Dirbrowse_RecursiveFree(files);
	files = NULL;
	file_count = 0;

	SceBool parent_dir_set = SCE_FALSE;

	if (R_SUCCEEDED(dir = sceIoDopen(cwd))) {
		int entryCount = 0;
		SceIoDirent *entries = (SceIoDirent *)calloc(MAX_FILES, sizeof(SceIoDirent));

		while (sceIoDread(dir, &entries[entryCount]) > 0)
			entryCount++;

		sceIoDclose(dir);
		qsort(entries, entryCount, sizeof(SceIoDirent), cmpstringp);

		for (int i = -1; i < entryCount; i++) {
			// Allocate Memory
			File *item = (File *)malloc(sizeof(File));
			memset(item, 0, sizeof(File));

			if ((strcmp(cwd, root_path)) && (i == -1) && (!parent_dir_set)) {
				strcpy(item->name, "..");
				item->is_dir = SCE_TRUE;
				parent_dir_set = SCE_TRUE;
				file_count++;
			}
			else {
				if ((i == -1) && (!(strcmp(cwd, root_path))))
					continue;

				item->is_dir = SCE_S_ISDIR(entries[i].d_stat.st_mode);

				// Copy File Name
				strcpy(item->name, entries[i].d_name);
				strcpy(item->ext, FS_GetFileExt(item->name));
				item->size = entries[i].d_stat.st_size;
				file_count++;
			}

			// New List
			if (files == NULL)
				files = item;

			// Existing List
			else {
				File *list = files;

				while(list->next != NULL)
					list = list->next;

				list->next = item;
			}
		}

		free(entries);
	}
	else
		return dir;

	if (!refresh) {
		if (position >= file_count)
			position = file_count - 1; // Keep index
	}
	else
		position = 0; // Refresh position

	return 0;
}

// ---- In-folder filename filter ----

static SceBool Dirbrowse_NameContains(const char *name, const char *query) {
	size_t name_len = strlen(name), query_len = strlen(query);
	if (query_len == 0)
		return SCE_TRUE;
	if (query_len > name_len)
		return SCE_FALSE;

	for (size_t i = 0; i + query_len <= name_len; i++) {
		if (!strncasecmp(name + i, query, query_len))
			return SCE_TRUE;
	}

	return SCE_FALSE;
}

static SceBool Dirbrowse_EntryVisible(File *file) {
	if (!strcmp(file->name, ".."))
		return SCE_TRUE;

	return Dirbrowse_NameContains(file->name, filter_query);
}

void Dirbrowse_SetFilter(const char *query) {
	snprintf(filter_query, sizeof(filter_query), "%s", query);
	position = 0;
}

void Dirbrowse_ClearFilter(void) {
	filter_query[0] = '\0';
	position = 0;
}

SceBool Dirbrowse_HasFilter(void) {
	return filter_query[0] != '\0';
}

const char *Dirbrowse_GetFilter(void) {
	return filter_query;
}

int Dirbrowse_GetVisibleCount(void) {
	int n = 0;
	for (File *file = files; file != NULL; file = file->next)
		if (Dirbrowse_EntryVisible(file))
			n++;
	return n;
}

File *Dirbrowse_GetFileIndex(int index) {
	int i = 0;
	for (File *file = files; file != NULL; file = file->next) {
		if (!Dirbrowse_EntryVisible(file))
			continue;
		if (i == index)
			return file;
		i++;
	}

	return NULL;
}

// ---- Rendering ----

#define LIST_X       (UI_RAIL_WIDTH)
#define ROW_H        64
#define ROW_ICON_SIZE 36
#define ROW_LIST_TOP 100
// Right edge a name may reach before the format badge starts.
#define ROW_BADGE_W   74
#define ROW_RIGHT_PAD 26

static void Dirbrowse_FormatSize(char *buf, int buf_size, SceOff bytes) {
	if (bytes >= 1024 * 1024)
		snprintf(buf, buf_size, "%.1f MB", bytes / (1024.0 * 1024.0));
	else if (bytes >= 1024)
		snprintf(buf, buf_size, "%.1f KB", bytes / 1024.0);
	else
		snprintf(buf, buf_size, "%d B", (int)bytes);
}

// Row type icons, drawn as geometry at the size the row wants rather than as
// PNGs rescaled into it. `size` is the glyph box, centred on (cx, cy).
typedef enum {
	ROW_ICON_DIR,
	ROW_ICON_AUDIO,
	ROW_ICON_FILE
} Dirbrowse_RowIcon;

#define ROW_GLYPH_SIZE 20

// Folder: a solid body with the raised tab along its top left.
static void Dirbrowse_DrawFolderGlyph(float cx, float cy, float size, unsigned int color) {
	float w = size, h = size * 0.78f;
	float x = cx - w / 2.0f, y = cy - h / 2.0f;
	float tab_h = h * 0.18f;

	UI_DrawQuad(x, y + tab_h, x + w * 0.34f, y + tab_h, x + w * 0.42f, y, x, y, color);
	UI_DrawRoundedRect(x, y + tab_h, w, h - tab_h, 3, color);
}

// Audio: a quaver - filled note head with a stem and a flag.
static void Dirbrowse_DrawAudioGlyph(float cx, float cy, float size, unsigned int color) {
	float x = cx - size / 2.0f, y = cy - size / 2.0f;
	float head_r = size * 0.19f;
	float head_cx = x + size * 0.30f, head_cy = y + size * 0.74f;
	float stem_x = head_cx + head_r * 0.92f;

	vita2d_draw_fill_circle(head_cx, head_cy, head_r, color);
	UI_DrawStroke(stem_x, head_cy, stem_x, y + size * 0.14f, size * 0.10f, color);
	UI_DrawStroke(stem_x, y + size * 0.14f, x + size * 0.82f, y + size * 0.30f, size * 0.10f, color);
}

// File: a page with the top right corner turned down.
static void Dirbrowse_DrawFileGlyph(float cx, float cy, float size, unsigned int color) {
	float w = size * 0.76f, h = size * 0.92f;
	float x = cx - w / 2.0f, y = cy - h / 2.0f;
	float fold = w * 0.36f;

	// The page is a pentagon - the corner is cut away rather than drawn over,
	// so the icon chip behind it shows through as the fold. Two pieces, because
	// the quad helper takes four points.
	UI_DrawQuad(x, y, x + w - fold, y, x + w, y + fold, x + w, y + h, color);
	UI_DrawTriangle(x + w, y + h, x, y + h, x, y, color);
}

static void Dirbrowse_DrawRowIcon(Dirbrowse_RowIcon icon, float cx, float cy, unsigned int color) {
	switch (icon) {
		case ROW_ICON_DIR: Dirbrowse_DrawFolderGlyph(cx, cy, ROW_GLYPH_SIZE, color); break;
		case ROW_ICON_AUDIO: Dirbrowse_DrawAudioGlyph(cx, cy, ROW_GLYPH_SIZE, color); break;
		case ROW_ICON_FILE: Dirbrowse_DrawFileGlyph(cx, cy, ROW_GLYPH_SIZE, color); break;
	}
}

static void Dirbrowse_DrawRow(File *file, float y, SceBool selected) {
	if (selected)
		UI_DrawRowHighlight(LIST_X + 10, y, 960 - LIST_X - 20, ROW_H);

	float icon_x = LIST_X + 22, icon_y = y + (ROW_H - ROW_ICON_SIZE) / 2;
	UI_DrawRoundedRect(icon_x, icon_y, ROW_ICON_SIZE, ROW_ICON_SIZE, 10, UI_COLOR_SURFACE_2);

	SceBool is_parent = !strcmp(file->name, "..");
	const char *badge_label = NULL; unsigned int badge_color = 0, badge_wash = 0;
	SceBool has_badge = (!file->is_dir) && UI_GetFormatBadge(file->ext, &badge_label, &badge_color, &badge_wash);

	Dirbrowse_DrawRowIcon(file->is_dir ? ROW_ICON_DIR : (has_badge ? ROW_ICON_AUDIO : ROW_ICON_FILE),
		icon_x + ROW_ICON_SIZE / 2.0f, icon_y + ROW_ICON_SIZE / 2.0f,
		selected ? ui_color_accent : UI_COLOR_TEXT_TERTIARY);

	const char *name = is_parent ? "Carpeta superior" : file->name;
	float text_x = LIST_X + 22 + ROW_ICON_SIZE + 12;
	float title_y = y + 10;
	// A name stops before the badge when the row has one and before the row's
	// right edge otherwise, instead of running through either.
	float name_w = (has_badge ? (960 - ROW_RIGHT_PAD - ROW_BADGE_W - 16) : (960 - ROW_RIGHT_PAD)) - text_x;

	UI_DrawTextClipped(UI_FACE_UI, UI_TS_BODY, text_x, UI_TextBaselineY(UI_FACE_UI, UI_TS_BODY, title_y, 24), name_w, UI_COLOR_TEXT_PRIMARY, name);

	if (!is_parent) {
		char subtitle[32];
		if (file->is_dir)
			snprintf(subtitle, sizeof(subtitle), "Carpeta");
		else
			Dirbrowse_FormatSize(subtitle, sizeof(subtitle), file->size);

		UI_DrawTextClipped(UI_FACE_MONO, UI_TS_LABEL, text_x, UI_TextBaselineY(UI_FACE_MONO, UI_TS_LABEL, title_y + 26, 20), name_w, UI_COLOR_TEXT_TERTIARY, subtitle);
	}

	if (has_badge)
		UI_DrawBadge(960 - ROW_RIGHT_PAD - ROW_BADGE_W, y + (ROW_H - 30) / 2, UI_TS_BADGE, badge_label, badge_wash, badge_color, badge_color);
}

void Dirbrowse_DisplayFiles(void) {
	int visible_count = Dirbrowse_GetVisibleCount();

	char caption[48];
	int folder_count = 0, track_count = 0;
	for (File *f = files; f != NULL; f = f->next) {
		if (!strcmp(f->name, ".."))
			continue;
		if (f->is_dir)
			folder_count++;
		else
			track_count++;
	}
	snprintf(caption, sizeof(caption), "%d CARPETAS . %d PISTAS", folder_count, track_count);
	UI_DrawText(UI_FACE_MONO, UI_TS_BADGE, LIST_X + 22, UI_TextBaselineY(UI_FACE_MONO, UI_TS_BADGE, 66, 26), UI_COLOR_TEXT_MUTED, caption);

	int printed = 0;

	for (int idx = 0; idx < visible_count; idx++) {
		if (printed == FILES_PER_PAGE)
			break;

		if (position < FILES_PER_PAGE || idx > (position - FILES_PER_PAGE)) {
			File *file = Dirbrowse_GetFileIndex(idx);
			Dirbrowse_DrawRow(file, ROW_LIST_TOP + (ROW_H * printed), idx == position);
			printed++;
		}
	}
}

void Dirbrowse_OpenFile(void) {
	char path[512];
	File *file = Dirbrowse_GetFileIndex(position);

	if (file == NULL)
		return;

	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);

	if (file->is_dir) {
		// Attempt to navigate to target
		if (R_SUCCEEDED(Dirbrowse_Navigate(SCE_FALSE))) {
			Dirbrowse_SaveLastDirectory();
			Dirbrowse_PopulateFiles(SCE_TRUE);
		}
	}
	else if ((!strncasecmp(file->ext, "flac", 4)) || (!strncasecmp(file->ext, "it", 4)) || (!strncasecmp(file->ext, "mod", 4))
		|| (!strncasecmp(file->ext, "mp3", 4)) || (!strncasecmp(file->ext, "ogg", 4)) || (!strncasecmp(file->ext, "opus", 4))
		|| (!strncasecmp(file->ext, "s3m", 4))|| (!strncasecmp(file->ext, "wav", 4)) || (!strncasecmp(file->ext, "xm", 4)))
		Menu_PlayAudio(path);
}

// Navigate to Folder
int Dirbrowse_Navigate(SceBool parent) {
	File *file = Dirbrowse_GetFileIndex(position); // Get index

	if (file == NULL)
		return -1;

	// Special case ".."
	if ((parent) || (!strncmp(file->name, "..", 2))) {
		char *slash = NULL;

		// Find last '/' in working directory
		int i = strlen(cwd) - 2; for(; i >= 0; i--) {
			// Slash discovered
			if (cwd[i] == '/') {
				slash = cwd + i + 1; // Save pointer
				break; // Stop search
			}
		}

		slash[0] = 0; // Terminate working directory
	}

	// Normal folder
	else {
		if (file->is_dir) {
			// Append folder to working directory
			strcpy(cwd + strlen(cwd), file->name);
			cwd[strlen(cwd) + 1] = 0;
			cwd[strlen(cwd)] = '/';
		}
	}

	Dirbrowse_ClearFilter();
	Dirbrowse_SaveLastDirectory();

	return 0; // Return success
}
