## 1. Base: módulo de idioma y config

- [x] 1.1 Crear `include/lang_strings.h` (X-macro), `include/lang.h` y `source/lang.c` con `Lang_Get`, `Lang_Resolve`, `Lang_Apply` y el idioma efectivo, según las decisiones 1 y 4 de `design.md`. Por ahora la tabla trae solo los textos de Ajustes. Agregar `lang.c` a `add_executable` en `CMakeLists.txt`. Contrastar el valor replicado de `SCE_SYSTEM_PARAM_LANG_SPANISH` con el header de VitaSDK. Listo cuando `scripts/build.sh --clean` compile con `-Werror` y `lang.c` no incluya headers de vita2d ni de SCE.
- [x] 1.2 Agregar pruebas en PC para `lang.c` (`tests/test_lang.c` más un `Makefile` o un target de CMake para el gcc del host), según la decisión 7. Cubren `Lang_Resolve` con las tres preferencias, un valor fuera de rango y varios idiomas de sistema, que ningún texto sea `NULL` o vacío, y que cada par tenga los mismos especificadores `%` en el mismo orden. Documentar el comando en el encabezado del test. Listo cuando las pruebas pasen en PC y fallen si se rompe a propósito un par de especificadores.
- [ ] 1.3 Migrar la config según la decisión 6: campo `language` en `config_t`, `CONFIG_VERSION` a 3, línea nueva al final del formato, lectura por cantidad de campos de `sscanf` con migración desde v2 sin reseteo, buffer de `Config_Save` de 256 bytes y truncado tratado como error. Listo cuando compile con `-Werror` y, en consola, una config v2 con sort Z-A, metadatos MP3 apagados y EQ Jazz se abra con esos valores intactos y quede reescrita como v3 con `language = 0`.
- [ ] 1.4 Resolver el idioma en `main.c`: leer `SCE_SYSTEM_PARAM_ID_LANG` después de `Utils_InitAppUtil()`, guardar el idioma del sistema y llamar a `Lang_Apply` antes de `Menu_DisplayFiles()`. Listo cuando compile con `-Werror` y la app arranque igual que antes en consola, todavía con los textos sin migrar.

## 2. Ajustes: textos y categoría Idioma

- [ ] 2.1 Pasar `source/menus/menu_settings.c` a la tabla: título, etiquetas de categoría como `LangString`, ítems y hints de Orden, Metadatos, Normalizador (sin el "Off"/"On" en español) y Ecualizador ("Apagado"/"Off"), y la barra de hints. Los nombres de dispositivos y de presets de EQ quedan como están. Listo cuando compile con `-Werror` y en consola Ajustes se vea en español neutro con acentos, igual en disposición que antes.
- [ ] 2.2 Agregar la categoría Idioma según la decisión 5: sexta entrada con System/Sistema, English y Español como radio, guardado con `Config_Save` y `Lang_Apply` inmediato, con hint del valor elegido. Listo cuando, en consola, elegir English redibuje Ajustes en inglés en el mismo fotograma sin cortar una pista que está sonando, reiniciar la app conserve la elección, y la lista de seis categorías entre sin tocar la barra de hints.

## 3. Resto de pantallas

- [ ] 3.1 Pasar a la tabla `source/menus/menu_displayfiles.c` y `source/dirbrowse.c`: hints, búsqueda, "Carpeta", "Carpeta superior" y el contador de carpetas y pistas con claves `_ONE`/`_MANY`. El título del IME se convierte de UTF-8 a `SceWChar16` al abrir el diálogo. Listo cuando compile con `-Werror` y en consola, en ambos idiomas, Carpetas, su contador (probado con 1 y con varias pistas) y el título del teclado de búsqueda salgan en el idioma activo.
- [ ] 3.2 Pasar a la tabla `source/menus/menu_library.c`: `view_label`, título, `UNKNOWN_LABEL`, contadores y plurales, hints de cada estado, y los avisos, con `Menu_LibraryNotice` guardando un `LangString` en vez de copiar texto (decisión 2). Listo cuando compile con `-Werror` y en consola, en ambos idiomas, se vean bien las cuatro vistas, la pantalla sin carpeta elegida, un aviso (por ejemplo, abrir un archivo borrado) y la entrada "Unknown"/"Desconocido" al final de Artistas, y cuando cambiar de idioma con un aviso en pantalla lo muestre ya traducido.
- [ ] 3.3 Pasar a la tabla las tres pantallas de progreso de `source/library.c` ("Escaneando la biblioteca", "Leyendo etiquetas", "Extrayendo carátulas", sus subtítulos, "Abandonar" y el aviso de límite), y `source/menus/menu_audioplayer.c` ("A CONTINUACIÓN" y los hints). Listo cuando compile con `-Werror` y en consola un reescaneo completo y la pantalla Reproduciendo se lean en ambos idiomas, con "Ó" bien dibujada en mayúscula.

## 4. Verificación integral

- [x] 4.1 Buscar literales que hayan quedado sin pasar a la tabla con un `grep` de cadenas entre comillas en `source/menus/`, `source/library.c`, `source/dirbrowse.c`, `source/mini_player.c` y `source/status_bar.c`, y justificar cada resto (rutas, formatos, nombres propios, depuración). Listo cuando no quede ningún texto visible fuera de la tabla, con la lista de restos anotada en esta tarea.
  Restos del `grep`, todos justificados:
  - Rutas y archivos: `ux0:data/ElevenMPV/...` en `library.c`, `dirbrowse.c` y `menu_settings.c`, y la marca `ELEVENMPV_LIBRARY` y las claves `root`/`count` del índice.
  - Nombres de dispositivos (`ux0:/`, `ur0:/`, `uma0:/`), de presets de EQ (Heavy, Pop, Jazz, Unique) y de idiomas (English, Español): nombres propios, iguales en ambos idiomas.
  - Extensiones de formato (`flac`, `mp3`, `opus`, etc.) y las entradas `.` y `..` del sistema de archivos.
  - Formatos sin palabras: la duración (`%02d:%02d`), el tamaño (`%.1f MB`, `KB`, `B`), el hint de metadatos (`%d/3`), la batería (`%d%%`), el separador del contador de Carpetas (`%s . %s`) y la hora de la barra de estado con `AM`/`PM`, que se usa igual en ambos idiomas.
  - Formatos del índice de la biblioteca (`"root\t%s\n"` y similares), que no se muestran.
- [ ] 4.2 Recorridos en consola de los scenarios de `specs/ui/language`, `specs/ui/settings` y `specs/library/index`:
  - Recorrido completo en English y en Español, revisando los textos recortados.
  - Primer arranque sin `config.cfg`, con la consola en español y en inglés.
  - Config con un `language` fuera de rango.
  - Actualización desde el `.vpk` de la v3.0.0 con ajustes cambiados.
  - Cambio de idioma con música sonando y con la biblioteca escaneada, sin reescaneo.

  Listo cuando todos los scenarios se cumplan en consola y las pruebas de PC de 1.2 sigan pasando.

## 5. Cierre

- [ ] 5.1 Agregar a `CHANGELOG.md`, en `[Sin publicar]`, los bullets de las notas de versión de la propuesta bajo `### Agregado` y `### Cambiado`, y subir `VITA_VERSION` a `"03.10"` en `CMakeLists.txt`. Listo cuando `changelog-section.sh "Sin publicar"` imprima los bullets, `check-version.sh v3.1.0` pase y el `.vpk` compilado tenga `APP_VER 03.10`.
- [ ] 5.2 Ejecutar `openspec validate add-language --strict`. Listo cuando pase y el check `build` del PR de `change/add-language` hacia `main` esté en verde.
