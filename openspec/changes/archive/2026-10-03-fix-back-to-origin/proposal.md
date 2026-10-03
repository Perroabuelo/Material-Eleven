Rama: `change/fix-back-to-origin`, creada desde `main`. Su PR apunta a `main`.

## Why

En Reproduciendo, el botón de volver de la consola (Círculo o Cruz, según cómo esté configurada) siempre lleva a Carpetas. Si la canción se eligió en la Biblioteca, el usuario pierde el lugar donde estaba y tiene que volver a navegar hasta su álbum o artista. Desde que existe la Biblioteca, "volver a Carpetas" ya no equivale a "volver de donde vine".

## What Changes

- **Volver desde Reproduciendo lleva a la pantalla donde se eligió la canción que suena.** Si se eligió en la Biblioteca, vuelve a la Biblioteca en la misma pestaña, dentro del mismo artista o álbum y con la misma fila seleccionada. Si se eligió en Carpetas, vuelve a Carpetas, como hasta ahora.
- **El origen se mantiene aunque se entre a Reproduciendo por otro camino.** Si el usuario reproduce desde la Biblioteca, pasa por Ajustes y vuelve a Reproduciendo con el nav rail, volver lo lleva a la Biblioteca y no a Ajustes.
- El origen cambia solo cuando el usuario elige otra canción para reproducir. Pasar a la siguiente pista o a la anterior no lo cambia.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `ui/nav-shell`: el botón de volver en Reproduciendo deja de llevar siempre a Carpetas. Lleva a la pantalla donde se eligió la canción.

## Impact

- **Código C**: `source/menus/menu_audioplayer.c`. Reproduciendo recuerda qué pantalla inició la reproducción (`Menu_PlayAudio` para Carpetas, `Menu_PlayQueued` para la Biblioteca), y el botón de volver lleva a esa pantalla.
- **APIs**: ninguna cambia de firma. `Menu_ShowNowPlaying` sigue igual.
- **Textos, assets, dependencias y build**: sin cambios.
- **CI**: sin cambios.

## Fuera de alcance

- Una pila de navegación general (volver desde Ajustes, desde la Biblioteca, etc.).
- Cambiar qué hace el botón de volver en las otras pantallas.
- Corregir que cada salto de pantalla se anida como una llamada más, sin deshacer la anterior. Se anota como riesgo en design.md.
- Recordar el origen entre sesiones de la app.

## Notas de version

Versión objetivo: **v3.3.1** (patch): corrige adónde lleva el botón de volver en Reproduciendo.

- The back button on Now Playing now returns you to the screen where you picked the song. From the Library, you land on the same tab, album or artist, and row you left.
