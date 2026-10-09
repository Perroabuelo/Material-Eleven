Rama: `change/refresh-readme-media`, creada desde `main`. Su PR apunta a `main`.

## Why

El README muestra seis capturas de antes de la 3.5. No se ven las letras sincronizadas ni el color de acento que cambia con cada portada, que son lo más vistoso de la app hoy. `refresh-livearea` dejó este rediseño anotado como "un cambio aparte". Ya hay capturas nuevas de la 3.5 y una grabación de la consola, y con ellas se armó un video de presentación. Falta llevar ese material al repo y dejar la forma de repetirlo en cada versión.

## What Changes

- **Video de presentación arriba del README.** Es un video de unos 28 s que se reproduce dentro de la página de GitHub y muestra la intro, la biblioteca, el cambio de color por portada, las letras sincronizadas y un cierre con el logo y la firma del autor. El `.mp4` no se versiona: se sube a GitHub y el README enlaza su URL `user-attachments`. Como respaldo, un GIF corto y mudo queda en `docs/media/`.
- **Capturas nuevas por versión.** La selección de capturas de la 3.5 va a `docs/screenshots/v3.5/`, con nombres descriptivos, y la tabla del README pasa a usarlas. Las capturas anteriores quedan donde están, en `docs/screenshots/`, como registro de cómo era la app, y el README enlaza a esa carpeta.
- **Herramienta para el video en `tools/promo/`.** `build.py` arma las versiones del video con ffmpeg a partir de una grabación de OBS:
  - GitHub, con y sin audio.
  - Completa, con y sin audio.
  - GIF y WebP.

  El montaje de cada grabación (qué tramo va en cada escena, los títulos y el color de acento de cada una) vive en un archivo de edición aparte. Las rutas de la grabación, la música y el avatar se pasan como argumentos, así que ningún archivo del repo tiene rutas de una máquina. `tools/README.md` explica cómo usarla, cómo corrige el rango de color de OBS y cómo grabar para que salga nítido.

## Capabilities

### New Capabilities

Ninguna. El cambio es de documentación y herramientas: no altera el comportamiento del reproductor, así que se declara `skip_specs: true`.

### Modified Capabilities

Ninguna.

## Fuera de alcance

- Versionar los `.mp4`. Las versiones con audio llevan canciones con derechos de autor y, además, GitHub no reproduce dentro del README un video guardado en el repo.
- Versionar música, grabaciones de OBS o el avatar del autor. `build.py` los recibe como argumentos.
- Publicar el video fuera de GitHub (YouTube, redes). Las versiones completas quedan listas, pero subirlas es decisión del autor.
- Traducir el README o los títulos del video. Siguen en inglés, como el resto del proyecto.
- Cambiar el banner de la LiveArea o `sce_sys/pic0.png`.
- Borrar, renombrar o mover las capturas anteriores.

## Notas de version

- Versión objetivo: **la próxima versión**, sin salto propio. El cambio no tiene efecto dentro de la app, así que no agrega bullets a `CHANGELOG.md` ni sube `VITA_VERSION` al archivarse.

## Impact

- **README:** `README.md`, con el video arriba, la tabla de capturas nueva y el enlace a las capturas anteriores.
- **Assets nuevos:** `docs/screenshots/v3.5/` (unas 8 capturas en JPG de 960×544) y `docs/media/material-eleven.gif`, de alrededor de 1.1 MB.
- **Herramientas nuevas:** `tools/promo/build.py`, el archivo de edición de la grabación de la 3.5 y una sección nueva en `tools/README.md`.
- **Código C, CI, APIs, textos y assets de la app:** sin cambios. `build.py` usa la biblioteca estándar de Python 3 y ffmpeg, y toma las fuentes de `res/` (Manrope e IBM Plex Mono, ambas con licencia SIL OFL 1.1, que ya figuran en `NOTICE`).
