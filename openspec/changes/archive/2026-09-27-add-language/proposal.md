Rama: `change/add-language`

## Why

Hoy toda la interfaz está en español, con los textos escritos directamente en el código. Las aplicaciones homebrew de PS Vita se distribuyen normalmente en inglés, y ElevenMPV, el proyecto original, también lo estaba, así que un usuario que no habla español no puede usar la app con comodidad. Además, el español actual es inconsistente: faltan acentos ("Atras", "Triangulo", "Albumes", "danado"), en otros lugares sí están ("Atrás", "Tamaño"), y hay voseo ("Elegí otra", "elegiste"). Conviene resolverlo ahora, antes de rehacer el README con capturas, para que esas capturas muestren la app en inglés.

## What Changes

- La interfaz queda disponible en dos idiomas, **English** y **Español**, y todos los textos visibles pasan a una única tabla de textos.
- El inglés es el idioma base: se usa cuando la consola no está en español y siempre que falte un texto.
- En el primer arranque la app sigue el idioma del sistema: si la consola está en español, la app sale en español; con cualquier otro idioma, sale en inglés.
- Ajustes suma una categoría **Idioma / Language** con tres opciones: Sistema, English y Español. El cambio se aplica al instante, sin reiniciar, y se guarda como cualquier otro ajuste.
- Los nombres de los idiomas se muestran siempre en su propio idioma ("English", "Español"), para que cualquiera pueda volver al suyo.
- Los textos en español se revisan a español neutro: sin voseo, con tuteo en las frases, infinitivo en los hints de botones, y acentos y ñ completos.
- La etiqueta de las pistas sin artista o sin álbum pasa a mostrarse en el idioma activo ("Unknown" / "Desconocido").
- Los textos que hoy mezclan idiomas pasan a un único idioma por pantalla. Por ejemplo, el Normalizador hoy muestra "Off"/"On" dentro de la UI en español.
- `config.cfg` gana el campo del idioma. Las configuraciones existentes se migran conservando todos sus valores, y el idioma queda en "Sistema". Hoy, un cambio de versión de la config borra todos los ajustes; este cambio evita ese reseteo.

## Capabilities

### New Capabilities
- `ui/language`: los idiomas de la interfaz. Cubre qué idiomas existen, cómo se elige el idioma inicial a partir del sistema, el inglés como respaldo, que todo texto visible salga del idioma activo, el cambio de idioma en vivo y la calidad del español (neutro y con ortografía completa).

### Modified Capabilities
- `ui/settings`: la pantalla consolidada suma una categoría de idioma. El requirement que enumera las categorías cambia para incluirla, y se agrega uno que exige que actualizar la config no borre los ajustes guardados.
- `library/index`: el requirement que agrupa bajo "Desconocido" las pistas sin artista o sin álbum pasa a describir esa entrada como una etiqueta en el idioma activo, sin cambiar el agrupamiento ni el orden al final de la lista.

## Impact

- **Código nuevo**: un módulo de textos (`include/lang.h`, `source/lang.c`) con la tabla English/Español, el idioma activo y la resolución del idioma del sistema. Es lógica pura, sin vita2d ni SCE, así que se puede probar en PC.
- **Código modificado**:
  - `source/menus/menu_library.c`, `menu_settings.c`, `menu_displayfiles.c` y `menu_audioplayer.c`: textos, hints y la categoría nueva.
  - `source/library.c` y `source/dirbrowse.c`: pantallas de escaneo y contadores.
  - `source/config.c` e `include/config.h`: campo del idioma, migración y buffer de escritura.
  - `source/main.c`: resolver el idioma una vez que SceAppUtil está inicializado.
  - `CMakeLists.txt`: agregar `lang.c`.
- **Config**: `CONFIG_VERSION` sube de 2 a 3, con una migración que conserva los ajustes existentes.
- **Índice de biblioteca**: sin cambios. La etiqueta "Desconocido" nunca se guardó en el índice, así que cambiar de idioma no obliga a reescanear.
- **Dependencias**: ninguna nueva. Las fuentes actuales (Manrope, IBM Plex Mono) ya dibujan acentos y ñ.
- **CI**: sin cambios en el pipeline.

## Fuera de alcance

- El README y sus capturas. Van en un cambio posterior, con capturas nuevas de la app en inglés.
- Idiomas distintos de English y Español, y archivos de idioma externos editables por la comunidad.
- Traducir los comentarios del código fuente, que hoy mezclan inglés y español.
- El overlay de depuración (por ejemplo, "sin MSAA") y los mensajes que no ve el usuario.
- Los nombres de géneros ID3 y de los presets de EQ (Heavy, Pop, Jazz, Unique), que son nombres propios y quedan iguales en ambos idiomas.
- El idioma del teclado del sistema (IME) y de los diálogos comunes, que ya siguen el idioma de la consola. Sí se traduce el título que la app le pasa al IME.
- Cambiar los separadores de los hints (". ") o el diseño visual de cualquier pantalla.

## Notas de version

Versión objetivo: **v3.1.0** (minor: funcionalidad nueva).

- La app ahora está disponible en inglés y en español.
- Al abrirla por primera vez, usa el idioma de tu consola: español si la consola está en español, inglés en cualquier otro caso.
- Nueva opción **Idioma** en Ajustes para elegir entre Sistema, English y Español, que se aplica al instante.
- Los textos en español se revisaron: ortografía completa y español neutro.
- Actualizar a esta versión conserva todos tus ajustes.
