## Why

El reskin visual (`add-ui-skin`) logró la estética buscada, pero heredó la convención preexistente de dibujar cada ícono y control como una textura PNG pequeña, reescalada en tiempo real con filtro bilineal. Eso produce controles borrosos y más chicos de lo previsto: el ícono del nav rail se reescala de 28px nativos a 19px mostrados, y `btn_rewind`/`btn_forward` (68px nativos) desbordan su propio botón de 44px. Los mockups de referencia en `templates/` dibujan en cambio cada ícono como forma vectorial (SVG), no como imagen. Además, la carátula del track en reproducción todavía no influye en el color de acento de la interfaz — algo que el propio "Reality Check" y el `design.md` archivado de `add-ui-skin` ya habían anticipado como deseable, pero quedó explícitamente diferido.

## What Changes

- Reemplazar los íconos de transporte (play, pause, forward, rewind, shuffle, repeat), los íconos del nav rail y el toggle de Ajustes — hoy texturas PNG cargadas con filtro bilineal — por dibujo vectorial (`vita2d_draw_array` con triángulos/quads para formas rellenas; segmentos rectos + `vita2d_draw_fill_circle` para los trazos curvos de shuffle/repeat).
- Reemplazar el atlas 9-slice de esquinas redondeadas (`ui_corner_sm`/`ui_corner_lg`) por arcos vectoriales (triangle fan), retirando la dependencia de texturas de `UI_DrawRoundedRect`.
- Retirar de `res/` y de `textures.h`/`textures.c` los recursos PNG de íconos de navegación/transporte, toggle y esquinas que quedan sin uso una vez completada la vectorización (se conservan los de batería y carátula, que no son controles de UI sino contenido/indicadores reales).
- Extraer un color de acento dinámico a partir del tono dominante de la carátula del track actual (`metadata.cover_image`), reemplazando el acento fijo `#FF9166` (hoy una constante de compilación) por un valor en runtime, aplicado por igual en las tres pantallas (Now Playing, Folders, Settings) y en el nav rail, incluido su indicador de pantalla activa.
- Cuando el track en reproducción no tiene carátula embebida (WAV, formatos tracker como MOD/IT/S3M), o no hay ningún track cargado, el acento cae al valor fijo `#FF9166` de siempre.
- La implementación se entrega en commits separados por etapas (ver `tasks.md`), no en un único commit — cada etapa deja el árbol en un estado compilable y funcional antes de pasar a la siguiente.

## Capabilities

### New Capabilities
- `ui/dynamic-accent`: el color de acento de toda la interfaz se deriva del tono dominante de la carátula del track en reproducción, con una regla de legibilidad/contraste sobre el color extraído y un fallback al color fijo cuando no hay carátula o no hay track cargado.

### Modified Capabilities
Ninguna. La técnica de dibujo (textura vs. primitiva vectorial) es un detalle de implementación: ningún requirement de `ui/nav-shell`, `ui/now-playing`, `ui/folder-browser` o `ui/settings` especifica cómo se dibuja un ícono, un control o una esquina, así que su comportamiento observable no cambia. Solo el color de acento pasa a ser observable y dinámico, de ahí la capability nueva.

## Impact

- **Código**: `source/textures.c`, `include/textures.h`, `source/ui_theme.c`, `include/ui_theme.h`, `source/nav_rail.c`, `source/menus/menu_audioplayer.c`, `source/menus/menu_settings.c`, `source/menus/menu_displayfiles.c`, y los tres decodificadores de metadata (`source/audio/flac.c`, `mp3.c`, `opus.c`) en el punto donde hoy se carga `metadata.cover_image`.
- **Recursos**: se eliminan de `res/` los PNG de íconos de navegación/transporte, toggle y esquinas 9-slice; se conservan los de batería y carátula por defecto.
- **Dependencia externa a verificar**: se asume disponible una función de dibujo por arreglo de vértices (`vita2d_draw_array` o equivalente) en el header de VITASDK instalado, y que las texturas de carátula se crean en layout lineal (legible directamente desde CPU) — ambos puntos se confirman como primer paso técnico, antes de tocar código de pantallas.
- **Decisiones revertidas**: este change revierte dos decisiones registradas en el `design.md` archivado de `add-ui-skin` — el uso de 9-slice para esquinas redondeadas, y el acento fijo (no dinámico) como no-goal explícito.
