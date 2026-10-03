#ifndef _ELEVENMPV_LIBRARY_ROW_H_
#define _ELEVENMPV_LIBRARY_ROW_H_

// En que posicion va cada campo de una fila del indice, segun su version.
//
// Es logica pura, sin vita2d ni SCE, para que leer un indice de la version
// anterior se pruebe en el PC con make -C tests.
//
//     size  mtime  tagged  ext  track  disc  title  artist  album  path   (v3)
//     size  mtime  tagged  ext               title  artist  album  path   (v2)
//
// La ruta va siempre la ultima: si alguna vez trae un tabulador, no corre los
// campos de detras.

#define LIBRARY_ROW_MAX_FIELDS 10

// Cada miembro es la posicion del campo en la fila, o -1 si esa version no lo
// trae. `fields` es cuantos campos tiene una fila completa.
//
// En v2 `tagged` es -1 aunque la fila lo lleve: esa marca dice que se leyeron
// los tags, pero no los numeros de pista, asi que una fila v2 se carga como
// pendiente para que la pasada de tags los complete.
typedef struct {
	int size, mtime, tagged, ext, track, disc, title, artist, album, path;
	int fields;
} LibraryRow_Layout;

// 1 si la version se reconoce y `out` queda lleno, 0 si no.
int LibraryRow_GetLayout(int version, LibraryRow_Layout *out);

#endif
