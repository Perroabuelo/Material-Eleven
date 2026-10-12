## Why

La pantalla táctil de la Vita solo sirve hoy para el nav rail, el mini reproductor, Reproduciendo y la caja de filtro de Carpetas. Las listas de Biblioteca, Carpetas y Ajustes, las pestañas de la Biblioteca y las acciones de los estados vacíos responden solo a botones. Además, la ventana visible de cada lista se calcula a partir de la fila seleccionada (páginas fijas de 5 en la Biblioteca, ventana deslizante en Carpetas), así que no existe un desplazamiento que el dedo pueda mover. El usuario espera poder recorrer y elegir con el dedo, como en cualquier pantalla táctil.

Este cambio se integra **después** de `key-repeat` y `library-search`: asume la repetición de ↑↓ al mantener, la búsqueda con △ (y su caja tocable) y el menú de □ de la Biblioteca, y les suma el toque sin cambiar lo que hacen.

Rama git: `change/touch-navigation`.

## What Changes

- Las listas de Biblioteca, Carpetas y Ajustes se desplazan arrastrando el dedo, con un desplazamiento libre en píxeles que ya no depende de la fila seleccionada. Al soltar con velocidad, la lista sigue moviéndose y frena sola (inercia).
- Un toque sobre una fila la resalta al apoyar el dedo y la activa al levantarlo, si el dedo no llegó a arrastrar: reproduce una pista, entra en un artista, álbum o carpeta, abre una categoría de Ajustes o cambia una opción. Un arrastre nunca activa nada.
- Las pestañas de la Biblioteca, las acciones de los estados vacíos ("Elegir carpeta", "Escanear", "Elegir otra"), la caja de búsqueda y el menú de □, y las categorías e items de Ajustes se pueden tocar.
- Con botones, la vista sigue a la selección como hoy. Si el usuario desplazó la lista con el dedo y la selección quedó fuera de la vista, la selección no cambia; el siguiente botón de dirección vuelve a mostrar la vista sobre ella.
- Las filas que se desplazan se recortan bajo la barra superior y sobre el mini reproductor, sin dibujarse encima de ellos.
- La lógica de gestos (toque o arrastre), del desplazamiento y de la inercia vive en módulos puros, con tests en PC.

## Capabilities

### New Capabilities
<!-- Ninguna. -->

### Modified Capabilities
- `ui/list-navigation`: se le agregan (ADDED) los requisitos de toque: desplazamiento con el dedo e inercia, toque que activa, controles tocables de Biblioteca, Carpetas y Ajustes, convivencia con los botones y con los demás elementos tocables (nav rail, mini reproductor). Esta capability la crea el cambio `key-repeat`, que va antes: este cambio solo puede archivarse después de archivar `key-repeat`, para que `openspec/specs/ui/list-navigation/spec.md` ya exista.

Las specs de cada pantalla (`library/views`, `ui/folder-browser`, `ui/settings`) no cambian: describen qué hace cada acción, no con qué se pide.

## Impact

- Código: `source/touch.c` / `include/touch.h` (capa de gestos encima), módulos puros nuevos para gestos y desplazamiento de listas, `source/menus/menu_library.c`, `source/dirbrowse.c` y `source/menus/menu_displayfiles.c`, `source/menus/menu_settings.c`.
- Dibujo: las tres listas pasan de una ventana derivada de la selección a un desplazamiento en píxeles con recorte.
- Pruebas: tests nuevos en `tests/` y su entrada en `tests/Makefile`; verificación en consola (Vita3K no sirve para el toque).
- Sin dependencias nuevas ni código de terceros.

## Fuera de alcance

- Mantener presionado (long-press) para acciones secundarias.
- Deslizar horizontalmente para cambiar de pestaña.
- Doble toque.
- Barra de desplazamiento rápido arrastrable en el borde.
- Cambiar cómo responden al toque el nav rail, el mini reproductor y Reproduciendo, salvo para que un arrastre de lista no los dispare.
- El panel táctil trasero.

## Notas de version

Versión objetivo: **v3.9.0** (minor: funcionalidad nueva).

- Library, Folders and Settings lists now scroll with your finger, and keep gliding after a flick.
- Tap a row to play it or open it; tap the Library tabs, empty-screen actions and Settings entries too.
- Scrolling with touch never plays anything by accident: only a tap does.
