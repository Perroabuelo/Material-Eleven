## Why

En las listas de la aplicación (Biblioteca, Carpetas, Ajustes) cada paso de la selección exige una pulsación nueva de la cruceta: mantener ↑ o ↓ no hace nada más que el primer paso. En una biblioteca de cientos o miles de canciones, recorrer la lista fila a fila a golpes de botón es lento y cansa, y es justo lo que cualquier usuario espera que funcione al mantener la tecla.

Rama git del cambio: `change/key-repeat`.

## What Changes

- Mantener ↑ o ↓ en una lista hace avanzar la selección sola: un primer paso inmediato, una pausa corta (unos 400 ms) y después un paso cada pocos fotogramas (unos 15 por segundo).
- Si se mantiene el botón un rato largo (unos 2 s), la repetición pasa a saltar una página de filas por paso, para poder cruzar listas largas.
- La repetición se detiene en el borde de la lista. Dar la vuelta (de la última fila a la primera y al revés) queda solo para una pulsación nueva, como hoy.
- Aplica a las listas: la de Biblioteca, la del navegador de Carpetas, la de elegir carpeta de la biblioteca y los ítems de Ajustes.
- No cambia nada fuera de las listas: en Reproduciendo ↑ sigue alternando la vista de letras una vez por pulsación, y L/R, ←/→, confirmar y volver siguen actuando una vez por pulsación.

## Capabilities

### New Capabilities
- `ui/list-navigation`: cómo se recorre la selección de una lista con la cruceta, incluida la repetición al mantener ↑/↓, la aceleración por páginas y el comportamiento en los bordes.

### Modified Capabilities
<!-- Ninguna: ninguna spec existente describe cómo avanza la selección con la cruceta. -->

## Impact

- `source/utils.c` / `include/utils.h`: `Utils_ReadControls` pasa a calcular, además del flanco `pressed`, una máscara de pasos repetidos para ↑/↓.
- Nuevo módulo de lógica pura (`source/key_repeat.c`, `include/key_repeat.h`) con su prueba en PC (`tests/test_key_repeat.c`), y su entrada en `CMakeLists.txt` y `tests/Makefile`.
- `source/menus/menu_library.c`, `source/menus/menu_displayfiles.c` (lista principal y elección de carpeta), `source/menus/menu_settings.c`: la navegación vertical usa los pasos repetidos.
- Sin dependencias nuevas ni código de terceros.

## Fuera de alcance

- Repetición de ←/→, L/R, confirmar, volver o cualquier otro botón.
- Repetición fuera de las listas (Reproduciendo, diálogos del sistema).
- Hacer configurables los tiempos de repetición desde Ajustes.
- Navegación táctil y búsqueda en la biblioteca: van en los cambios `touch-navigation` y `library-search`.

## Notas de version

Versión objetivo: **v3.7.0** (minor: funcionalidad nueva).

- Holding Up or Down in a list now keeps moving the selection, so you no longer have to press once per row.
- Keep holding and the list starts jumping a page at a time, to get through long libraries quickly.
- Holding stops at the top or bottom of the list; a new press still wraps around.
