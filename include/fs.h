#ifndef _ELEVENMPV_FS_H_
#define _ELEVENMPV_FS_H_

#include <psp2/types.h>

SceBool FS_FileExists(const char *path);
SceBool FS_DirExists(const char *path);
const char *FS_GetFileExt(const char *filename);

// Si esa extension es una de las que la aplicacion sabe reproducir. Vive
// aqui y no repetida en cada pantalla porque library/index exige que la
// biblioteca indexe exactamente lo que el navegador sabe abrir: dos listas
// separadas se separan mas tarde o temprano.
SceBool FS_IsPlayableExt(const char *ext);
int FS_GetFileSize(const char *path, SceOff *size);
int FS_ReadFile(const char *path, void *buf, int size);
int FS_WriteFile(char *path, void *buf, int size);

#endif
