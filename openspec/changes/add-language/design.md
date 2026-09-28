## Context

La motivación está en `proposal.md`, sección Why. Estado actual del código:

- **Los textos visibles están escritos como literales** en unos siete archivos, cerca de 85 en total:
  - `source/menus/menu_library.c` (~28), `menu_settings.c` (~25), `menu_displayfiles.c` (~10) y `menu_audioplayer.c` (5).
  - `source/library.c`, en las tres pantallas de progreso de escaneo.
  - `source/dirbrowse.c`: "Carpeta", "Carpeta superior" y el contador "%d CARPETAS . %d PISTAS".
- **Casi todos se resuelven al dibujar.** Cada pantalla se redibuja en cada fotograma y toma el texto en ese momento, por ejemplo con los arreglos `hints[]` o las llamadas a `UI_DrawText(... "Ajustes")`. Hay tres excepciones:
  - Arreglos `static const char *[]` con etiquetas: `view_label` en `menu_library.c:52`, y en `menu_settings.c` `categories[]`, `sort_items`, `sort_hints`, `alc_items`, `eq_items` y `meta_items`.
  - `Menu_LibraryNotice` (`menu_library.c:121`) **copia** el texto del aviso a un buffer en el momento del evento, y lo muestra durante `NOTICE_FRAMES`.
  - El título del IME de búsqueda es un literal UTF-16 (`u"Buscar en esta carpeta"`, `menu_displayfiles.c:126`).
- **Hay plurales y formatos dentro de `snprintf`**: `"%d pista"` y `"%d pistas"`, `"%d de %d"`, los contadores con sufijo (`menu_library.c:242-273`), `"Limite de %d pistas alcanzado"` en `library.c:820` y el contador de `dirbrowse.c:318`.
- **La etiqueta del cubo de "Desconocido" es solo de presentación.** El índice no la guarda (`include/library.h:97`) y la vista la decide con `Library_NameIsUnknown(i)` (`menu_library.c:248`), así que el orden y el agrupamiento no dependen del texto.
- **La config tiene dos defectos** que este cambio no puede dejar como están:
  - `Config_Load` lee con un único `sscanf` sobre un formato fijo, y si `config_ver < CONFIG_VERSION` **borra el archivo y resetea todos los ajustes** (`config.c:72-84`). Subir la versión con esa lógica le borraría los ajustes a cada usuario que actualice.
  - `Config_Save` escribe en un `malloc(128)`. El archivo actual ocupa unos 126 bytes, así que una línea más quedaría truncada por `snprintf` sin error visible.
- **Orden de arranque** (`main.c:76-92`): `Config_Load()` y `Library_Load()` corren antes de `Utils_InitAppUtil()`, y la primera pantalla recién se dibuja en `Menu_DisplayFiles()`. `sceAppUtilSystemParamGetInt` necesita AppUtil inicializado, y ya se usa así para el volumen (`vitaaudiolib.c:80`).
- **Las fuentes** ya dibujan acentos y ñ: en consola, "Atrás" y "Tamaño" se ven bien. `UI_DrawText` recibe UTF-8.

## Goals / Non-Goals

**Goals:**

- Que agregar o cambiar un texto se haga en **un solo archivo**, y que la compilación falle si a un texto le falta uno de los dos idiomas.
- Que ningún camino deje un texto "congelado" en el idioma anterior después de cambiar el idioma en Ajustes.
- Que la lógica de idioma sea pura y se pueda probar en PC.
- Que actualizar desde v3.0.0 no borre los ajustes de nadie.

**Non-Goals:**

- Un sistema general de i18n: formatos de fecha y número por región, reglas de plural por idioma o carga de idiomas en tiempo de ejecución. Con dos idiomas y plurales de dos formas no hace falta.
- Traducir textos que no ve el usuario (overlay de depuración, logs) ni los comentarios del código.
- Reescribir el parser de la config más allá de lo que exige la migración.

## Decisions

### 1. Una tabla X-macro con los dos idiomas en la misma línea

Un encabezado `include/lang_strings.h` declara cada texto una sola vez:

```c
X(STR_SETTINGS_TITLE, "Settings", "Ajustes")
X(STR_TRACKS_ONE,     "%d track", "%d pista")
X(STR_TRACKS_MANY,    "%d tracks", "%d pistas")
```

`include/lang.h` expande esa lista en un `enum LangString`, y `source/lang.c` la expande en dos arreglos `const char *[]`, uno por idioma. La consulta es `Lang_Get(STR_...)`.

Por qué: la macro exige tres argumentos, así que **no se puede agregar un texto sin su traducción**, porque no compila. Además, el enum y los arreglos se generan de la misma lista y no pueden desalinearse. Leer inglés y español en la misma línea también facilita revisar la traducción.

*Alternativa descartada:* dos arreglos con inicializadores designados (`[STR_X] = "..."`). Un texto olvidado queda como `NULL` sin ningún aviso del compilador, y aparece recién como un texto vacío en consola.

*Alternativa descartada:* archivos de idioma en `ux0:`. Suman parser, manejo de errores de E/S y un formato que mantener, para un beneficio (traducciones de terceros) que está fuera de alcance.

### 2. Las etiquetas se guardan como IDs y se resuelven al dibujar

Todo lugar que hoy guarda un `const char *` con texto visible pasa a guardar un `LangString`, y llama a `Lang_Get` en el momento de dibujar:

- Los arreglos estáticos de etiquetas y el campo `label` de `SettingsCategory`.
- `Menu_LibraryNotice`, que guarda el ID del aviso en vez de copiar el texto.
- `UNKNOWN_LABEL`, que pasa a ser `Lang_Get(STR_UNKNOWN)`.

Por qué: así el cambio en vivo que pide la spec `ui/language` se cumple solo, sin un mecanismo de "invalidar y reconstruir textos". Cada fotograma ya redibuja todo, y `Lang_Get` es un acceso a un arreglo.

*Alternativa descartada:* resolver los textos al entrar a cada pantalla. Obliga a acordarse de refrescar en cada lugar, y ese es justamente el tipo de olvido que deja un texto en el idioma viejo.

Los textos con formato se guardan en la tabla con sus especificadores (`"%d of %d"` / `"%d de %d"`) y se usan como formato de `snprintf`. `-Wall` no incluye `-Wformat-nonliteral`, así que compila con `-Werror`. La coherencia de los especificadores entre idiomas se asegura con la prueba de PC de la decisión 7 y con revisión.

### 3. Plurales con dos claves

Cada texto con cantidad tiene una clave `_ONE` y otra `_MANY`, y el código elige con `n == 1`, como ya lo hace `menu_library.c:242`. En inglés y en español la regla es la misma (1 frente al resto, y 0 va con el plural), así que no hace falta una función de plural por idioma.

### 4. Preferencia e idioma efectivo son dos valores distintos

- `config.language` guarda la **preferencia**: `0 = Sistema`, `1 = English`, `2 = Español`.
- `lang.c` guarda el **idioma efectivo**, que es `EN` o `ES`.
- `Lang_Resolve(preferencia, idioma_del_sistema)` es una función pura:
  - Preferencia `1` da `EN` y preferencia `2` da `ES`.
  - Preferencia `0`, o cualquier valor fuera de rango, da `ES` si el idioma del sistema es `SCE_SYSTEM_PARAM_LANG_SPANISH`, y `EN` en cualquier otro caso.
- El valor de `SCE_SYSTEM_PARAM_LANG_SPANISH` (3) se replica como constante en `lang.h`, para que `lang.c` no incluya headers de SCE. La tarea de verificación lo contrasta con el header de VitaSDK.
- `main.c` lee `SCE_SYSTEM_PARAM_ID_LANG` **después** de `Utils_InitAppUtil()` y aplica el idioma antes de `Menu_DisplayFiles()`. Nada se dibuja antes, así que no hay un fotograma en el idioma equivocado.
- El idioma del sistema leído en el arranque se conserva, para poder recalcular cuando el usuario vuelve a elegir "Sistema".

### 5. Categoría Idioma en Ajustes

- Es una sexta entrada de `categories[]`, después de Ecualizador, con tres ítems de radio.
- Activar un ítem guarda `config.language`, llama a `Config_Save` y recalcula el idioma efectivo. El siguiente fotograma ya se dibuja en el idioma nuevo.
- La etiqueta de la categoría es `STR_LANGUAGE` ("Language" / "Idioma").
- Los ítems son `STR_LANG_SYSTEM` ("System" / "Sistema"), y "English" y "Español" como literales fijos, que no pasan por la tabla porque son iguales en ambos idiomas.
- El valor que se ve en la lista de categorías es el nombre corto de la opción elegida: "System"/"Sistema", "English" o "Español".
- La lista de categorías tiene lugar: con seis filas de `CAT_ROW_H = 56` desde `HEADER_H + 10`, la última termina en y = 410, por encima de la barra de hints.

### 6. Migración de la config sin reseteo

- `CONFIG_VERSION` pasa a 3, y el formato suma al final la línea `language = %d`.
- `Config_Load` usa el valor de retorno de `sscanf`, que es la cantidad de campos leídos:
  - Si `config_ver` es 2 y se leyeron los 9 campos anteriores, conserva esos valores, pone `language = 0` y reescribe el archivo.
  - Si es 3, lee los 10 campos.
  - Si la versión es menor que 2, o el archivo no se puede leer, mantiene el reseteo a los valores por defecto que existe hoy.
- `Config_Save` pasa de un buffer de 128 bytes a uno de 256, y trata un `snprintf` truncado como error en vez de escribir un archivo cortado.
- Un `language` fuera de rango no se corrige al cargar: `Lang_Resolve` ya lo trata como Sistema, y la categoría de Ajustes lo muestra como Sistema.

*Alternativa descartada:* no subir la versión y aprovechar que `sscanf` deja `language` sin tocar si falta la línea. Funciona hoy por casualidad, pero deja el formato del archivo sin versionar para la próxima migración.

### 7. Estrategia de pruebas

- **Compilación**: `-Wall -Werror` limpio en cada tarea. La X-macro convierte "texto sin traducción" en error de compilación.
- **Pruebas en PC**: `lang.c` es puro, así que se agregan pruebas en PC compiladas con el gcc del host (por ejemplo, `tests/test_lang.c` con un `Makefile` o un target de CMake aparte). Cubren:
  - `Lang_Resolve` con las tres preferencias, un valor fuera de rango y varios idiomas de sistema.
  - Que ningún texto sea `NULL` o vacío en ningún idioma.
  - Que cada par de textos tenga los mismos especificadores `%`, en el mismo orden.
- **Consola**: los recorridos descritos en los scenarios de `ui/language`, `ui/settings` y `library/index`:
  - Recorrido completo en cada idioma.
  - Cambio en vivo con música sonando.
  - Arranque sin config con la consola en español y en inglés.
  - Actualización desde la v3.0.0 con ajustes cambiados.
  - Config con un `language` inválido.
  - Revisión de los textos que se recortan con `UI_DrawTextClipped`, en ambos idiomas.

### 8. CI

No cambia el pipeline: `build.yml` compila el `.vpk`, y `lang.c` entra en `add_executable`. Las pruebas de PC se agregan al repositorio, pero no se conectan al CI en este cambio. Conectarlas sería un cambio de pipeline aparte, pendiente desde que se definió que los tests de PC se agregan cuando existan.

### 9. Licencias

No se incorpora código ni assets de terceros. Los textos nuevos son propios, bajo GPL-3.0-or-later.

## Risks / Trade-offs

- [Un texto en inglés más largo que su espacio] → Casi todos los textos se dibujan con `UI_DrawTextClipped` o tienen espacio de sobra, y el inglés suele ser más corto. Los que se dibujan con `UI_DrawText` sin recorte (títulos y avisos de escaneo) se revisan en el recorrido por consola.
- [Especificadores `%` distintos entre idiomas] → Hay prueba en PC que compara los especificadores de cada par, y la revisión de la tabla es lado a lado.
- [Algún literal queda sin pasar a la tabla] → La tarea final incluye un `grep` de literales entre comillas en los archivos de UI, y el recorrido completo por consola en inglés hace visible cualquier resto en español.
- [La migración de config falla con un archivo editado a mano] → Si `sscanf` no lee los campos que exige su versión, se cae al reseteo que existe hoy. Nunca se usa un valor a medio leer.
- [El título del IME necesita UTF-16] → Se convierte el texto UTF-8 de la tabla a `SceWChar16` en el momento de abrir el diálogo, con una función chica de conversión. Los textos de la tabla caben en el BMP, así que no hacen falta pares sustitutos.
- [Mayúsculas con acento en las etiquetas en mayúsculas ("A CONTINUACIÓN", "CARPETAS")] → Se escriben en mayúscula directamente en la tabla, sin `toupper`, que no maneja UTF-8. La verificación en consola confirma que IBM Plex Mono dibuja "Ó".

## Migration Plan

- Para el usuario, la actualización es transparente: la config v2 se migra a v3 conservando sus valores, con idioma "Sistema". La biblioteca y las carátulas no cambian.
- Rollback: si se reinstala la v3.0.0 sobre una config v3, `sscanf` de la versión vieja lee `config_ver = 3`, que no es menor que 2, así que no resetea. Ignora la línea `language` y conserva el resto. No hace falta ningún paso manual.
