> **Entrega en commits por partes.** Cada grupo numerado de abajo es al menos un commit, y ningún grupo se mezcla con otro en el mismo commit. Todo commit deja el árbol compilable con `scripts/vita.ps1` y la aplicación utilizable en consola, para que un fallo sea atribuible por bisección. Los grupos 1 a 6 van en ese orden; el 7 solo se escribe si el grupo 0 lo autoriza.

## 0. Medición previa (sin código de producción)

- [x] 0.1 Medir sobre una colección real el tiempo de solo recorrer la carpeta con `sceIoDread`, sin abrir ningún archivo, y anotar pistas encontradas y segundos transcurridos
- [x] 0.2 Medir el tiempo añadido de leer tags de MP3, OGG, OPUS y FLAC, y de leer el nombre de módulo con `xmp_test_module`, comparando contra 0.1 sobre la misma colección
- [x] 0.3 Medir el costo de extraer y decodificar una carátula embebida, y determinar si `vita2d_load_JPEG_buffer` usa el decodificador JPEG por hardware de la consola o una ruta por software, revisando la implementación de vita2d en el toolchain
- [x] 0.4 Registrar los tres números y la conclusión sobre la fase 3 en `design.md`, y fijar ahí el techo de pistas y el tamaño de miniatura que las Open Questions dejaron abiertos; verificar que la decisión de escribir o retirar el grupo 7 queda escrita

## 1. Acotar las lecturas de directorio existentes

- [x] 1.1 Acotar el bucle de lectura de `Dirbrowse_PopulateFiles` (`source/dirbrowse.c:69`) para que no escriba más allá de `MAX_FILES`, y verificar con una carpeta de prueba de más de 1024 entradas que la aplicación lista lo que cabe y sigue navegando sin caerse
- [x] 1.2 Acotar igual el bucle de `Menu_GetMusicList` (`source/menus/menu_audioplayer.c:41`), y verificar que reproducir un archivo desde esa misma carpeta de prueba arranca y construye cola sin caídas
- [x] 1.3 Verificar en consola que el comportamiento en carpetas normales no cambió: entrar, salir, filtrar y reproducir se comportan igual que antes del cambio

## 2. La cola pasa a tener dueño (refactor sin cambio de comportamiento)

- [x] 2.1 Crear el módulo de cola con su lista de rutas, su posición y su API de llenado, y verificar que compila sin que nadie lo use todavía
- [x] 2.2 Mover la construcción de cola por carpeta de `Menu_GetMusicList` al productor de carpeta del módulo nuevo, sin alterar qué archivos entran ni en qué orden, y verificar que compila
- [x] 2.3 Sustituir `playlist[1024][512]` estático por memoria dimensionada a la cola real, y verificar que el binario ya no reserva esos 512 KB de forma permanente
- [x] 2.4 Verificar en consola que la cola es idéntica a la de antes del refactor: misma carpeta, mismo orden de pistas, y que siguiente, anterior, aleatorio y repetición se comportan igual, incluida la previsualización de próximas pistas de Now Playing

## 3. Fase 1 — recorrido, índice y persistencia

- [x] 3.1 Implementar el recorrido con pila explícita de carpetas pendientes y profundidad acotada, sin reservar arreglos de entradas de directorio, y verificar contra una estructura de carpetas anidada de prueba que encuentra todas las pistas reproducibles y ninguna que no lo sea
- [x] 3.2 Implementar el troceado del escaneo por fotograma con dibujado de progreso y abandono, siguiendo la forma del lazo de `Menu_PromptFilter`, y verificar en consola que el progreso avanza visiblemente y que abandonar devuelve el control
- [x] 3.3 Implementar la escritura y lectura del índice en texto con cabecera de versión, carpeta y número de pistas, y verificar reiniciando la aplicación que la biblioteca aparece sin reescanear
- [ ] 3.4 Implementar el descarte del índice cuya versión no se reconoce o que está incompleto, y verificar con un archivo de índice alterado a mano que la aplicación ofrece reescanear en vez de mostrar datos parciales
- [x] 3.5 Persistir la carpeta de escaneo en su propio archivo junto a `lastdir.txt`, sin tocar `CONFIG_VERSION`, y verificar que tras actualizar sobre una instalación previa los ajustes de ecualizador, orden y dispositivo siguen intactos
- [x] 3.6 Implementar el techo de pistas con truncado informado, y verificar contra una colección que lo supere que la biblioteca queda en el máximo, lo informa y sigue respondiendo
- [ ] 3.7 Implementar la tolerancia a archivos y carpetas ilegibles, y verificar con un archivo de audio deliberadamente dañado que el escaneo continúa y el resto de la colección queda indexado

## 4. Fase 1 — la pantalla de biblioteca

- [x] 4.1 Añadir la cuarta destinación a `UI_Screen` y al nav rail con su glifo vectorial, y verificar en consola que aparece en las cuatro pantallas, que marca la activa y que su área táctil alcanza el mínimo que exige `ui/rendering`
- [x] 4.2 Implementar la pantalla de biblioteca con la lista plana de canciones y sus cuatro estados —sin carpeta, escaneando, vacía tras escanear, con contenido— y verificar cada estado en consola provocándolo
- [x] 4.3 Implementar la elección de carpeta reutilizando el navegador en modo selección, y verificar que confirmar elige la carpeta actual en vez de reproducir, y que cancelar vuelve sin cambiar nada
- [ ] 4.4 Implementar reescanear y cambiar de carpeta desde la pantalla de biblioteca, y verificar que añadir y borrar archivos en la carpeta se refleja tras reescanear, y que elegir otra carpeta no deja ninguna pista de la anterior
- [x] 4.5 Conectar el productor de biblioteca a la cola, y verificar que reproducir desde la lista de canciones toma esa lista como cola y que siguiente avanza por ella
- [ ] 4.6 Verificar que la reproducción en curso no se corta durante un escaneo completo, y que abrir una pista cuya ruta ya no existe avisa sin caerse y sugiere reescanear
- [x] 4.7 Verificar que el navegador de carpetas sigue comportándose igual que antes del change, incluido reproducir archivos que están fuera de la carpeta de escaneo
- [x] 4.8 Mostrar el mini reproductor en la pantalla de biblioteca reutilizando el del navegador en vez de escribir un segundo, y verificar que refleja la pista en curso, que sus controles responden y que la lista sigue cabiendo

## 5. Fase 2 — tags

- [x] 5.1 Implementar la segunda pasada del escaneo, que recorre el índice leyendo tags y persiste cada cierto número de pistas, y verificar interrumpiéndola que lo leído queda guardado y que la siguiente pasada continúa por las pistas pendientes
- [x] 5.2 Implementar la lectura de título, artista y álbum para MP3, OGG, OPUS y FLAC, y verificar contra archivos de cada formato con tags conocidos que los tres campos llegan a la biblioteca
- [x] 5.3 Implementar el título de los módulos de tracker con `xmp_test_module`, y verificar contra un módulo con nombre interno conocido que la biblioteca muestra ese nombre y no el del archivo
- [x] 5.4 Implementar el respaldo por nombre de archivo para WAV y para archivos sin tags, y verificar con un MP3 sin ID3 y con un WAV que ambos aparecen con su nombre de archivo
- [x] 5.5 Verificar que un archivo con cabecera de tags dañada no interrumpe la pasada y queda con su respaldo por nombre de archivo

## 6. Fase 2 — vistas

- [x] 6.1 Implementar las vistas de artistas y de álbumes sobre el índice, con artista y álbum vacíos agrupados bajo "Desconocido" puesto por la vista y no guardado en el índice, y verificar que un artista realmente llamado "Desconocido" no se mezcla con el cubo
- [x] 6.2 Verificar que "Desconocido" queda al final de ambas listas y no intercalado alfabéticamente
- [x] 6.3 Implementar la vista de recientes a partir de la fecha de modificación, con el criterio de desempate elegido, y verificar que el orden va de lo más reciente a lo más antiguo
- [x] 6.4 Implementar el cambio entre las cuatro vistas y la entrada a un artista y a un álbum, y verificar que se navega entre ellas sin salir de la pantalla de biblioteca
- [x] 6.5 Verificar que reproducir desde un álbum repartido en varias carpetas encadena las pistas del álbum y no las de la carpeta del archivo que suena
- [x] 6.6 Verificar que la previsualización de próximas pistas de Now Playing muestra la cola de la vista de biblioteca cuando la reproducción salió de ahí, y la de la carpeta cuando salió del navegador

## 7. Fase 3 — carátulas (solo si el grupo 0 la autorizó)

- [ ] 7.1 Implementar la caché de miniaturas en disco indexada por álbum, con píxeles crudos a tamaño fijo, y verificar que un álbum de varias pistas con la misma carátula produce una sola entrada de caché
- [ ] 7.2 Conectar la extracción existente de MP3, FLAC y OPUS a la caché, y verificar que las pistas de esos formatos con carátula embebida la muestran en las vistas
- [ ] 7.3 Implementar el marcador por defecto para pistas sin carátula, incluidas las de OGG, WAV y tracker, y verificar que no se reintenta la extracción en cada arranque
- [ ] 7.4 Verificar que las carátulas aparecen tras reiniciar sin volver a abrir los archivos de audio, y que recorrer rápidamente una lista larga no detiene el desplazamiento esperando imágenes
- [ ] 7.5 Verificar que el acento dinámico derivado de la carátula del track en reproducción sigue comportándose como exige `ui/dynamic-accent`, sin interferencia de la caché de la biblioteca

## 8. Cierre

- [ ] 8.1 Ejecutar `openspec validate add-music-library --strict` y verificar que pasa
- [ ] 8.2 Si el grupo 0 descartó la fase 3, retirar sus requirements de `specs/library/index/spec.md` y `specs/library/views/spec.md` y el grupo 7 de este archivo, y verificar que la validación sigue pasando
- [ ] 8.3 Verificar en consola un recorrido completo sobre una colección real: escanear, recorrer las cuatro vistas, reproducir desde una vista y desde una carpeta, reescanear tras añadir y borrar archivos, y reiniciar la aplicación
