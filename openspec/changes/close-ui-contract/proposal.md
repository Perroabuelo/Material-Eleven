## Why

El reskin (`add-ui-skin`) y la vectorización (`vectorize-ui-controls`) dejaron la interfaz nueva en pie, pero tres defectos verificados impiden darla por cerrada: el texto se dibuja borroso por un comportamiento del motor de fuentes de vita2d, la escala tipográfica completa está ~1.38x por debajo del tamaño físico que el lenguaje de diseño asume, y las formas vectoriales se rasterizan sin antialiasing porque la aplicación inicializa vita2d sin MSAA. A eso se suma que quedan dos islas de PNG (batería e íconos de fila), que no existe cobertura de glifos no latinos, y que ningún spec vigente dice una palabra sobre tipografía, antialiasing, densidad ni targets táctiles — así que cada pantalla nueva puede reintroducir los mismos defectos sin violar nada.

Además, `vectorize-ui-controls` produjo dos crasheos de GPU en hardware (`ac8f53d`, `f3d908e`), ambos de la misma clase: la CPU liberó o reutilizó memoria que la GPU todavía estaba leyendo. Este change amplía exactamente las mismas superficies, así que el ciclo de vida de los recursos GPU pasa a ser parte del contrato en vez de una convención oral.

## What Changes

- **Instrumentación primero.** Antes de tocar nada visual se agregan las defensas que la historia del repositorio demuestra que hacen falta: un único punto de destrucción de recursos GPU que sincroniza siempre, validación de los retornos de inicialización y carga, un contador observable de agotamiento del pool de vértices, y símbolos de depuración (`-g`) para que los volcados `psp2core` resuelvan números de línea.
- **Un handle de fuente por cada tamaño dibujado.** El atlas de `vita2d_font` se indexa solo por índice de glifo, sin el tamaño: cada glifo se rasteriza una única vez, al tamaño en que apareció primero, y todo otro tamaño es un reescalado bilineal de ese bitmap. Compartir un handle entre tamaños es la causa del texto borroso.
- **Antialiasing por MSAA.** `vita2d_init()` se reemplaza por `vita2d_init_advanced_with_msaa(...)`, con degradación a inicialización simple si el modo pedido no entra en memoria.
- **Escala tipográfica corregida y consolidada**: de 7 tokens (11–22 px) a 5 tokens (15–30 px), aplicando el factor de densidad real de la pantalla de la consola.
- **Densidad recalculada** como consecuencia aceptada del punto anterior: alto de fila, targets táctiles y alto de la barra de hints suben; la lista de carpetas pasa de 7 a 5 filas visibles.
- **Política de overflow de texto**, que hoy no existe en ninguna pantalla: los nombres largos se desbordan sin truncado, elipsis ni marquee.
- **Cobertura de glifos no latinos** mediante la fuente del sistema como respaldo por rango de codepoint, para que los nombres de archivo y tags en japonés, chino o cirílico se dibujen en vez de quedar en blanco.
- **Cierre de las islas de PNG**: la batería pasa a una forma vectorial con nivel (reemplazando 15 texturas) y los íconos de fila (`icon_dir`, `icon_file`, `icon_audio`) pasan a vectorial.
- **Purga de recursos muertos**: se retiran de `res/` y de la carga los recursos que hoy se cargan en cada arranque y no se dibujan nunca (`default_artwork_blur`, `default_artwork`, `Roboto-Regular.ttf`, `radio_button_*`, `icon_back`), más el bucket de batería que la escalera de porcentaje nunca alcanza.
- **Line baseline desde métricas de la cara**, no del bounding box del string, para que las líneas base dejen de desplazarse entre filas según qué letras contiene cada texto.
- **Entrega por etapas, siempre en commits por partes.** La implementación se entrega en commits separados por etapa (ver `tasks.md`), nunca en un commit único. Cada etapa deja el árbol compilable y funcional, y se prueba en hardware por separado antes de pasar a la siguiente, de modo que un crasheo sea atribuible a una etapa concreta por bisección.

## Capabilities

### New Capabilities
- `ui/typography`: cómo se dibuja el texto en toda la aplicación — escala en píxeles de dispositivo con tamaño mínimo, un handle de fuente por tamaño, línea base derivada de métricas de la cara, política de overflow para texto que no cabe, y cobertura de glifos para contenido no latino.
- `ui/rendering`: cómo se dibuja todo lo demás — antialiasing de las formas vectoriales, prohibición de texturas rasterizadas para chrome de interfaz, tamaño táctil mínimo de los controles, y las invariantes de ciclo de vida de recursos GPU que evitan la clase de crasheo ya observada dos veces.

### Modified Capabilities
Ninguna. Se revisaron los cinco specs vigentes (`ui/nav-shell`, `ui/folder-browser`, `ui/now-playing`, `ui/settings`, `ui/dynamic-accent`) y ninguno especifica tamaños de texto, densidad de filas, antialiasing, targets táctiles ni cobertura de glifos. Sus requirements se cumplen igual antes y después: el nav rail sigue listando las mismas tres destinaciones, la lista de carpetas sigue mostrando los mismos badges de formato, y las tres pantallas siguen compartiendo el acento derivado de la carátula. Lo que cambia es cómo se dibuja todo eso, y eso es precisamente lo que las dos capabilities nuevas pasan a cubrir.

## Impact

- **Código**: `source/main.c` (inicialización y validación), `source/ui_theme.c` e `include/ui_theme.h` (tokens tipográficos, handles de fuente, baseline, primitivas, punto único de destrucción GPU), `source/status_bar.c` (batería vectorial y reloj), `source/dirbrowse.c` (íconos de fila, alto de fila, overflow), `source/nav_rail.c` (targets táctiles), `source/menus/menu_audioplayer.c`, `source/menus/menu_displayfiles.c` y `source/menus/menu_settings.c` (relayout por la escala nueva), `source/textures.c` e `include/textures.h` (purga), `CMakeLists.txt` (flags de depuración y recursos).
- **Recursos**: se retiran de `res/` los PNG de batería, los íconos de fila, los radio buttons, `icon_back`, `default_artwork`, `default_artwork_blur` y `Roboto-Regular.ttf`. Puede incorporarse un peso adicional de Manrope si la verificación en hardware lo justifica.
- **Memoria GPU**: sube por dos vías simultáneas — los atlas de fuente (uno por tamaño) y el modo MSAA. Es la única superficie del change donde dos decisiones compiten por el mismo recurso, y se verifica antes de dar por cerrada la primera etapa.
- **Dependencias externas verificadas**: contra el header y la biblioteca instalados en el toolchain se confirmó que `vita2d_init_advanced_with_msaa`, el recorte rectangular (`vita2d_set_clip_rectangle`) y las fuentes del sistema (`vita2d_load_system_pvf`) existen, y se confirmó por desensamblado el comportamiento del atlas que motiva el handle por tamaño. Lo que no pudo verificarse sin consola — cobertura real de las fuentes del firmware y costo del MSAA en fotogramas por segundo — queda como tarea de verificación en hardware, no como supuesto del diseño.
- **Decisiones revertidas**: ninguna. Este change extiende `vectorize-ui-controls` a las dos superficies que dejó fuera a propósito (batería e íconos de fila) y corrige un comentario del código que afirma que vita2d no ofrece recorte, cuando sí lo ofrece.
- **Fuera de alcance, confirmado con el usuario**: transiciones entre pantallas, efecto ripple, las pantallas Library / Playlists / Queue, scroll con inercia, tema claro y acento configurable a mano.
