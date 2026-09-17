#ifndef _ELEVENMPV_DIRBROWSE_H_
#define _ELEVENMPV_DIRBROWSE_H_

#include <psp2/types.h>

typedef struct File {
	struct File *next; // Next item
	SceBool is_dir;        // Folder flag
	char name[256];    // File name
	char ext[5];       // File extension
	SceOff size;       // File size in bytes (0 for folders / the ".." entry)
} File;

extern File *files;

int Dirbrowse_PopulateFiles(SceBool clear);
void Dirbrowse_DisplayFiles(void);
// Index into the filter-visible sequence (see Dirbrowse_SetFilter).
File *Dirbrowse_GetFileIndex(int index);
void Dirbrowse_OpenFile(void);
int Dirbrowse_Navigate(SceBool parent);

// In-folder filename filter (no new directory reads): narrows
// Dirbrowse_GetFileIndex / Dirbrowse_DisplayFiles / Dirbrowse_GetVisibleCount
// to name-matching entries. The ".." parent entry is always visible.
// Dirbrowse_Navigate clears the filter whenever cwd actually changes.
void Dirbrowse_SetFilter(const char *query);
void Dirbrowse_ClearFilter(void);
SceBool Dirbrowse_HasFilter(void);
const char *Dirbrowse_GetFilter(void);
int Dirbrowse_GetVisibleCount(void);

#endif
