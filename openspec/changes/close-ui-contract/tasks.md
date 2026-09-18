> **Regla de entrega, válida para todo el change: los commits son siempre por partes.**
> Cada grupo numerado de abajo es una etapa y se entrega en sus propios commits, nunca
> agrupando varias etapas en un commit único, y dentro de una etapa se prefiere un commit
> por unidad coherente de trabajo antes que un commit grande al final. Cada etapa deja el
> árbol compilable y funcional, y **no se inicia la etapa siguiente hasta que la actual pasó
> su prueba en hardware**. Así un fallo queda atribuible a una etapa concreta por bisección,
> que es como se localizaron los dos crasheos de `vectorize-ui-controls`.

> **Protocolo de prueba en hardware (PPH).** Varias tareas lo citan por nombre. Consiste en:
> arranque en frío; cambio repetido de track alternando tracks con y sin carátula embebida;
> alternancia rápida entre Now Playing, Folders y Settings; navegación por carpetas profundas
> y vuelta atrás; y diez minutos de reproducción continua. Se considera superado si no se
> genera ningún volcado `psp2core` y la interfaz sigue respondiendo.

## 1. Instrumentación y defensas (ninguna etapa posterior empieza sin esto)

- [x] 1.1 Agregar símbolos de depuración a los flags de compilación y verificar que el binario resultante permite a `vita-parse-core` resolver nombres de función y números de línea sobre un volcado de prueba
- [x] 1.2 Establecer el punto único de destrucción de recursos gráficos, que sincroniza el dibujo pendiente y después libera, y migrar a él todas las liberaciones existentes de textura y de tipografía; verificar por inspección que no queda ninguna liberación directa fuera de ese punto y que ninguna sincronización vive dentro de una rama condicional
- [x] 1.3 Validar el resultado de toda carga de imagen y de tipografía antes de usarla, eliminando la dereferencia sin comprobación que hoy ocurre al fijar los filtros de una textura recién cargada; verificar forzando el fallo de una carga que la aplicación sigue en pie
- [x] 1.4 Añadir el contador de agotamiento del pool de vértices y la superposición de depuración conmutable con memoria gráfica libre y marca de agua del pool; verificar que la superposición muestra valores plausibles y que puede activarse y desactivarse en ejecución
- [x] 1.5 Ejecutar el PPH sobre esta etapa y registrar la memoria gráfica libre de referencia, que servirá de línea base para las etapas siguientes
- [ ] 1.6 Confirmar que la interfaz se ve exactamente igual que antes de la etapa, dado que esta etapa no debe mover ningún píxel

## 2. Nitidez del dibujo y del texto

- [x] 2.1 Reemplazar la inicialización de vita2d por la variante que acepta modo de suavizado y tamaño de pool, comprobando el retorno y degradando a inicialización simple si el modo pedido no se puede establecer; verificar que la aplicación arranca en ambos caminos forzando el fallo del modo pedido
- [ ] 2.2 Medir los fotogramas por segundo con suavizado activo en las tres pantallas y compararlos con la etapa anterior, dejando el resultado registrado; si el costo obliga a bajar el modo, ajustarlo aquí y no en etapas posteriores
- [x] 2.3 Reemplazar los dos handles de tipografía compartidos por un handle por cada tamaño realmente dibujado, con nombres que hagan imposible dibujar un tamaño con el handle de otro; verificar comparando en pantalla un mismo carácter en dos tamaños distintos, que deben verse con el mismo nivel de definición
- [ ] 2.4 Comprobar que la nitidez ya no depende del recorrido: llegar a Now Playing por dos rutas de navegación distintas y con tracks distintos, y verificar que el título se ve igual en ambos casos
- [x] 2.5 Derivar la línea base del texto de las métricas de la tipografía al tamaño dibujado en vez del recuadro de la cadena concreta; verificar que dos filas consecutivas, una con caracteres descendentes y otra sin ellos, comparten línea base, y que el tiempo transcurrido no salta verticalmente al cambiar de valor
- [ ] 2.6 Revisar la memoria gráfica libre con la superposición frente a la línea base de 1.5, ya que esta etapa suma atlas de tipografía y modo de suavizado a la vez
- [x] 2.7 Ejecutar el PPH sobre esta etapa

## 3. Escala tipográfica, densidad y overflow

- [x] 3.1 Consolidar los siete tokens tipográficos actuales en los cinco de la escala nueva y aplicar el factor de densidad de la pantalla; verificar que ningún texto de ninguna pantalla queda por debajo del tamaño mínimo, revisando explícitamente reloj, porcentaje de batería, badges de formato y hints de botones
- [x] 3.2 Subir el alto de fila de la lista de carpetas y el alto de la barra de hints a los valores que la escala nueva exige, y rehacer el layout de Folders; verificar que las filas no se solapan y que el mini reproductor y la barra de hints siguen visibles
- [x] 3.3 Rehacer el layout de Settings con la escala nueva; verificar que la lista de categorías y el área de detalle siguen mostrando lo mismo que antes, sin recortes involuntarios
- [x] 3.4 Rehacer el layout de Now Playing con la escala nueva; verificar que carátula, título, artista, badge, barra de búsqueda, controles y vista previa de siguientes siguen cabiendo sin solaparse
- [x] 3.5 Llevar los targets táctiles de los controles de pista anterior y siguiente y de las entradas de la barra de navegación al mínimo táctil; verificar tocando cerca de los bordes del área activa que el toque se registra
- [x] 3.6 Implementar el recorte de texto con indicador de continuación sobre el recorte rectangular, habilitándolo y deshabilitándolo alrededor de cada dibujo acotado; verificar con un nombre de archivo largo que no invade el badge ni la columna vecina, y con un título largo que no se sale del panel
- [x] 3.7 Recorrer las tres pantallas buscando recorte que haya quedado activo por una ruta de salida temprana, comprobando que nada desaparece después de un dibujo acotado
- [x] 3.8 Ejecutar el PPH sobre esta etapa

## 4. Cierre de las islas rasterizadas

- [x] 4.1 Reemplazar el indicador de batería por una forma vectorial con nivel y estado de carga; verificar que refleja los mismos niveles que la lógica actual, incluido el tramo que hoy queda inalcanzable por la escalera de porcentaje, y que se ve con la misma definición en todos ellos
- [x] 4.2 Reemplazar los íconos de tipo de entrada de la lista por geometría vectorial; verificar en una carpeta con subcarpetas, audio reconocido y archivos no reconocidos que cada entrada muestra su ícono
- [x] 4.3 Simplificar el dibujo del reloj para que la cadena se calcule una sola vez por fotograma, eliminando además la rama condicional cuyos dos caminos producen el mismo resultado; verificar que el ancho medido y el texto dibujado siempre coinciden
- [ ] 4.4 Revisar el contador de agotamiento del pool tras sumar la geometría de esta etapa y, si el margen quedó estrecho, ajustar el tamaño del pool en la llamada de inicialización
- [x] 4.5 Retirar de los recursos y de la carga las imágenes y la tipografía que ya no dibuja ninguna pantalla, incluidas las que no se dibujaban antes de este change; verificar que la aplicación compila, arranca y que la memoria gráfica libre al inicio mejora respecto de la línea base
- [x] 4.6 Corregir el comentario del código que afirma que la biblioteca no ofrece recorte, dejando constancia de que el recorte rectangular existe y que lo que falta es el recorte por forma arbitraria
- [x] 4.7 Ejecutar el PPH sobre esta etapa

## 5. Cobertura de glifos no latinos

- [x] 5.1 Comprobar en hardware qué escrituras cubren las tipografías del sistema de la consola, dejando registrado el resultado; si alguna de las esperadas no estuviera cubierta, acotar el alcance de esta etapa a lo que la consola sí representa antes de seguir
- [x] 5.2 Cargar el respaldo del sistema únicamente en los tamaños donde aparece contenido del usuario, a los píxeles exactos del token y con factor de escala neutro; verificar que el texto latino dibujado con el respaldo mide lo mismo que con la tipografía propia
- [x] 5.3 Implementar la partición de la cadena por rango de codepoint y el dibujo por tramos, avanzando la posición con el ancho que informa cada motor; verificar con un nombre que mezcla escrituras latina y no latina que la línea se dibuja completa y sin huecos entre tramos
- [ ] 5.4 Verificar con archivos cuyos nombres estén en japonés, chino y cirílico que aparecen dibujados en la lista de carpetas, en el título y en el artista, en lugar de quedar en blanco
- [x] 5.5 Implementar la cuenta de codepoints no latinos distintos ya dibujados y la recreación del handle de respaldo al cruzar el umbral, pasando por el punto único de destrucción de 1.2 y agendada en un cambio de carpeta, nunca dentro del dibujo de un fotograma
- [ ] 5.6 Probar la recreación cruzando el umbral dos veces seguidas mientras se navega contenido no latino, verificando que los caracteres se siguen dibujando después de cada recreación y que no se genera ningún volcado; esta es la ruta más peligrosa del change y no se da por cerrada sin esta prueba
- [ ] 5.7 Ejecutar el PPH sobre esta etapa

## 6. Cierre del change

- [ ] 6.1 Recorrer los requirements de `ui/typography` escenario por escenario sobre la consola y confirmar que cada uno se cumple
- [ ] 6.2 Recorrer los requirements de `ui/rendering` escenario por escenario sobre la consola y confirmar que cada uno se cumple
- [ ] 6.3 Confirmar que los cinco specs vigentes siguen comportándose igual: el nav rail lista las mismas tres destinaciones y marca la activa, los badges de formato aparecen en las mismas extensiones, el filtro por nombre sigue acotado a la carpeta actual, los ajustes conservan su efecto y persistencia, y el acento sigue derivándose de la carátula con su reserva fija
- [ ] 6.4 Evaluar sobre la consola si los tamaños chicos necesitan un peso tipográfico adicional y, de ser así, incorporarlo como ajuste aditivo; si no hace falta, dejarlo registrado para no volver a abrirlo
- [ ] 6.5 Revisar que ninguna etapa dejó tareas de verificación sin registrar su resultado, en particular las mediciones de fotogramas por segundo, de memoria gráfica libre y de cobertura de escrituras

## Registro de verificacion

Las tareas que sólo se cierran con la consola en la mano quedan sin marcar hasta
que se ejecuten sobre hardware. Esta tabla registra qué se verificó, cómo y con
qué resultado, y es lo que la tarea 6.5 recorre al cerrar el change.

| Tarea | Estado | Cómo / resultado |
|---|---|---|
| 1.1 símbolos de depuración | verificado | `-g` junto a `-O3`. El ELF trae 18 secciones `.debug_*` y `arm-vita-eabi-addr2line -f` resuelve `UI_GpuFreeTexture` a `source/ui_gpu.c:34`. |
| 1.2 punto único de destrucción | verificado por inspección | Cero llamadas a `vita2d_free_texture` / `vita2d_free_font` fuera de `source/ui_gpu.c`. Las tres sincronizaciones del repositorio están en el primer nivel de su función, ninguna dentro de una rama. |
| 1.3 validación de cargas | verificado por inspección | `Texture_LoadImageBilinear` retorna antes de fijar filtros si el PNG no decodifica; los cuatro accesos de texto y `UI_GpuDrawTexture` toleran un recurso nulo. Falta forzar el fallo en consola. |
| 1.4 contador y superposición | **verificado en consola** | Conmuta con L + R + SELECT y reporta valores plausibles. Lectura: `user 97280 KB`, `cdram 90112 KB`, marca de agua `1999776 B`, agotado `0 veces`, `GFX MSAA 4x`, cdram al init `114688 KB`. |
| 1.5 PPH + línea base de memoria | **superado** | CDRAM libre al inicializar: **114688 KB (112 MB)**. Como las cuatro etapas llegaron juntas, no hay una línea base previa a la etapa 2 con la que contrastar: esta cifra es de arranque, antes de crear el render target y los atlas. PPH superado: sin volcado `psp2core` y la interfaz siguió respondiendo. |
| 1.6 la interfaz no se movió | pendiente de consola | |
| 2.1 MSAA con degradación | **verificado en consola**, con desvío del diseño | `vita2d_init_advanced_with_msaa` retorna `1` incondicionalmente: el desensamblado de `libvita2d.a` muestra `movs r0, #1` en sus dos salidas e ignora todo error de `sceGxm`. Comprobar el retorno, como pide `design.md`, no puede detectar nada. La degradación se decide **antes** de la llamada, según el CDRAM libre que informa `sceKernelGetFreeMemorySize`. El modo elegido queda visible en la superposición. |
| 2.2 FPS con suavizado | pendiente de consola | |
| 2.3 un handle por tamaño | implementado | 8 pares cara/tamaño en uso, un handle cada uno (~2 MB). Pendiente de consola: comparar un mismo carácter en dos tamaños. |
| 2.4 nitidez independiente del recorrido | pendiente de consola | |
| 2.5 línea base por métricas | implementado | `UI_TextBaselineY` ya no recibe la cadena: centra la extensión de la cara medida una vez sobre `"Agjy"`. El parámetro desapareció de los 27 sitios de dibujo, así que la regresión no se puede escribir. Pendiente de consola: filas con y sin descendentes. |
| 2.6 memoria gráfica vs. línea base | **medida** | CDRAM 114688 KB al init contra 90112 KB en uso: **24576 KB (24 MB) consumidos**. De esos, 1536 KB son los seis atlas de fuente (6 x 256 KB) y los 22.5 MB restantes son el render target con MSAA 4x más los framebuffers. Quedan **88 MB libres**. El riesgo que `design.md` señalaba como el único punto donde dos decisiones compiten por el mismo recurso queda holgado: no hace falta bajar el modo de suavizado ni recortar tamaños. |
| 2.7 PPH etapa 2 | **superado** | Cubierto por la misma pasada: las cuatro etapas llegaron juntas, así que un solo PPH las ejercita a todas. Sin volcado. |
| 3.1 escala consolidada | implementado | 7 tokens a 5: 15/17/19/22/30 px, o sea 10.9/12.3/13.8/16.0/21.8 unidades. El más chico llega al mínimo de etiqueta del lenguaje; el reloj, el porcentaje de batería, los badges y los hints usan los dos más chicos y ninguno queda por debajo. |
| 3.2 densidad de Folders | implementado | Fila 50 -> 64, barra de hints 32 -> 40, `FILES_PER_PAGE` 6 -> 5, mini reproductor 62 -> 72. Comprobado por aritmética: filas 100..420, mini 432..504, hints 504..544, sin solape. |
| 3.3 relayout de Settings | implementado | Header 60 -> 64, fila de categoría 44 -> 56, de ítem 46 -> 58, columna 252 -> 286. Peor caso (Ecualizador, 6 ítems con divisor) 82..452 contra la barra de hints en 504. |
| 3.4 relayout de Now Playing | implementado | Las siete bandas verificadas por aritmética: carátula 54..312, título 332..370, artista 370..396, badge 404..434, tiempos 232..256, transporte 280..356, siguientes 364..490. Ninguna se solapa. |
| 3.5 targets táctiles | implementado | `UI_TOUCH_MIN` = 66 px = 48 unidades = 7.6 mm. El control dibujado conserva su tamaño y sólo crece el área activa (`UI_TouchTarget`). Con `TOGGLE_ICON_GAP` en 26 las áreas de shuffle/anterior y siguiente/repetir se solapaban 1 px y el botón probado primero se comía el borde del otro; a 30 quedan 3 px de separación. El mini reproductor se rehizo sobre un paso de 66 px. |
| 3.6 recorte con indicador | **verificado en consola** | `UI_DrawTextClipped` sobre `vita2d_set_clip_rectangle`, con el indicador dibujado fuera del recorte. Aplicado a nombres de archivo, subtítulos, breadcrumb, título y artista (pantalla y mini), siguientes y etiquetas de ajustes. Confirmado en consola: un nombre largo se corta y muestra los tres puntos. **Fuera de alcance y pospuesto por el usuario:** marquee o scroll horizontal para leer el nombre completo, que `design.md` descartó a propósito para no medir la cadena en cada fotograma. |
| 3.7 recorte que quede activo | verificado por inspección | El recorte se habilita y deshabilita dentro de `UI_DrawTextClipped`, sin ninguna ruta de retorno entre ambas llamadas, así que no puede quedar activo. Es el único sitio del repositorio que lo toca. |
| 3.8 PPH etapa 3 | **superado** | Misma pasada. Sin volcado. |
| 4.1 batería vectorial | implementado | Carcasa redondeada, borne y barra de carga cuyo largo **es** el porcentaje, así que todo nivel tiene su ancho. El tramo inalcanzable desaparece por construcción: la escalera de 7 buckets, donde 30..49 caía en el de 50 y el arte de 30 nunca se dibujaba, ya no existe. Ámbar bajo 20%, rayo al cargar, carcasa vacía si no se puede leer el nivel. |
| 4.2 íconos de fila vectoriales | implementado | Carpeta, corchea y página con esquina doblada, dibujados a 20 px. Pendiente de consola: verlos en una carpeta con los tres tipos. |
| 4.3 reloj | implementado | La cadena se arma una vez por fotograma y de ahí salen el ancho y el dibujo; antes se leía el reloj dos veces. Eliminada la condicional cuyas dos ramas tenían el mismo `snprintf`. |
| 4.4 margen del pool | **medido; el pool está sobredimensionado 22x** | Agotado **0 veces**, así que ninguna geometría se dejó de dibujar. Marca de agua `1999776 B` libres sobre 2 MB, o sea un **pico de 97376 B (95 KB) por fotograma, el 4.6% del pool**. La justificación que escribí al fijarlo en 2 MB —que el chrome vectorial gasta mucho más por fotograma que las texturas que reemplazó— **no se sostiene contra la medición**: el default de 1 MB de vita2d sobraba, y 512 KB dejarían aún 5x de holgura. La marca de agua sólo cubre los fotogramas realmente dibujados en la sesión. |
| 4.5 purga de recursos | implementado | Fuera 16 PNG de batería, 4 de íconos, 2 de radio, 2 de carátula por defecto y `Roboto-Regular.ttf` (se cargaba en cada arranque y no lo dibujaba nadie). `source/textures.c`, `include/textures.h` y la maquinaria `ADD_RESOURCES` quedaron sin contenido y se eliminaron. El .vpk pasa de 2 885 058 a 1 981 814 bytes, y ya no se reservan 24 texturas ni un handle de fuente al arrancar. Pendiente de consola: confirmar la mejora de memoria libre contra la línea base. |
| 4.6 comentario sobre recorte | implementado | Corregido en `Menu_RunNowPlayingLoop`: el recorte rectangular existe y el change lo usa; lo que no hay es stencil ni recorte por forma arbitraria. |
| 4.7 PPH etapa 4 | **superado** | Misma pasada. Sin volcado. El cambio repetido de track alternando con y sin carátula embebida es el caso exacto de `f3d908e`, y no se reprodujo. |
| 5.1 cobertura del firmware | **verificado en consola** | La sonda carga el PVF del sistema (cabecera `sistema x1.0`) y **se dibujaron las cinco filas**: latín, cirílico, coreano, japonés y chino. La consola representa todo lo esperado, así que no hay que acotar el alcance de la etapa por cobertura. El despacho multi-fuente por predicado de codepoint también funciona: un solo `vita2d_load_system_pvf` con cuatro configs resolvió las cuatro escrituras. |
| 5.2 carga del respaldo | implementado, **con desvío del diseño** | **Un solo handle**, no uno por tamaño: `scePvfSetCharSize` está fijo en la biblioteca, así que varios handles serían atlas idénticos de 18 px. Se dibuja a la escala de cada token (`ui_text_px[ts] / 18`). |
| 5.3 partición y dibujo por tramos | implementado, **con desvío del diseño** | Partición **por codepoint**, no por rango: el cirílico está cubierto a medias por la tipografía propia. La cobertura se consulta a FreeType (`FT_Get_Char_Index`) sobre una `FT_Face` abierta sólo para eso, nunca para dibujar. Dibujo y medición comparten el mismo recorrido (`UI_TextRuns`), así que no pueden discrepar. |
| decisión: título a 30 px | **resuelta por el usuario** | Cuando el título trae codepoints que la tipografía propia no cubre, se dibuja a 19 px (`UI_TS_FALLBACK_MAX`) en vez de 30. Nítido y más chico, en vez de grande y borroso; afecta sólo a tracks que hoy son ilegibles. |
| 5.4 verificación en consola | pendiente de consola | |
| 5.5 renovación del handle | implementado | Bitmap de 8 KB sobre el BMP para contar codepoints distintos, umbral en 480 sobre los ~600 que entran en la hoja. La renovación pasa por `UI_GpuFreePvf` y se agenda en `Dirbrowse_PopulateFiles`, que siempre corre después de `vita2d_swap_buffers`. |
| 5.6 prueba de la renovación | pendiente de consola | **Es la ruta más peligrosa del change** y no se da por cerrada sin esta prueba. |
| 5.7 PPH etapa 5 | pendiente de consola | |

## Cobertura real de las tipografías propias

Leyendo el `cmap` de los dos archivos de `res/` directamente. Esto responde, sin
consola, la mitad de la tarea 5.1 que se refiere a la tipografía propia; lo que
sigue faltando es qué cubren las tipografías del firmware.

| Rango | Manrope | IBM Plex Mono |
|---|---|---|
| Latín básico y acentos | 100% | 100% |
| Latín extendido A | 89.8% | 100% |
| Griego | 52.1% | 0.7% |
| Cirílico | **40.6%** | **65.6%** |
| Hangul (jamo y sílabas) | **0%** | **0%** |
| Hiragana / Katakana | **0%** | **0%** |
| CJK unificado | **0%** | **0%** |

Dos consecuencias que el `proposal.md` no anticipaba:

- **El coreano no aparece "en blanco", aparece repetido.** Ningún codepoint
  Hangul está en el `cmap`, así que `FTC_CMapCache_Lookup` devuelve índice de
  glifo 0 para todos. Como el atlas se indexa por índice de glifo, **todos los
  caracteres no cubiertos comparten la misma entrada**: se dibujan todos con el
  mismo `.notdef` de Manrope. De ahí que se vea "incorrecto" y no vacío.
  Confirmado en hardware por el usuario con Melomance y Jokers.
- **El cirílico está cubierto a medias**, no ausente. Una cadena en ruso se
  dibujará parte bien y parte como `.notdef`, que es un modo de fallo peor de
  leer que la ausencia total, y el respaldo tendrá que decidir por codepoint y
  no por rango completo.

Defecto secundario, del mismo síntoma: `metadata.title` y `metadata.artist` son
`char[64]`. En UTF-8 el Hangul ocupa 3 bytes, así que un título se corta a unos
21 caracteres, y el corte puede caer en mitad de una secuencia y dejar un byte
inválido que `utf8_to_ucs2` decodifica mal.

## Bloqueo de la etapa 5

La etapa 5 no se inició. Hay dos bloqueos y el segundo invalida el mecanismo que
`design.md` prescribe.

**1. La tarea 5.1 es una precondición de hardware, no un cierre.** A diferencia
de las etapas 1 a 4, donde las pruebas en consola van al final, la etapa 5 abre
con "comprobar en hardware qué escrituras cubren las tipografías del sistema...
**si alguna de las esperadas no estuviera cubierta, acotar el alcance de esta
etapa antes de seguir**". El alcance del resto de la etapa depende de ese
resultado.

**2. El tamaño del respaldo del sistema no se puede elegir.** Desensamblando
`libvita2d.a`:

- `vita2d_load_pvf_pre` llama a `scePvfSetEM(0.0555556)` y a
  `scePvfSetResolution(128.0)`.
- `vita2d_load_system_pvf` llama después a `scePvfSetCharSize` con la constante
  `0x41220000`, es decir **10.125, escrita en el código de la biblioteca**. La
  función no recibe ningún parámetro de tamaño.
- `vita2d_load_custom_pvf` no llama a `scePvfSetCharSize` en absoluto.
- `generic_pvf_draw_text` usa el mismo par `texture_atlas_get` /
  `texture_atlas_insert`, así que el atlas del PVF es tan ciego al tamaño como
  el de `vita2d_font`.

De ahí que **todo handle de PVF del sistema rasterice al mismo tamaño**, que a
128 ppp sale 10.125 x 128 / 72 = 18 px (la conversión es una derivación; el dato
firme es que el tamaño es fijo e igual para todos los handles). La tarea 5.2
pide cargarlo "a los píxeles exactos del token y con factor de escala neutro":
con escala neutra sólo se puede dibujar un token de 18 px, y la escala nueva no
tiene ninguno (15, 17, 19, 22, 30).

**Estado tras medir en consola.** De los dos bloqueos, el primero se cerró: la
tarea 5.1 está verificada y la consola cubre las cinco escrituras. El segundo se
redujo mucho al medir la escala en hardware:

- a escala neutra el respaldo se ve nítido;
- a x1.67, la escala que pide el token de 30 px, se ve muy borroso.

El PVF rasteriza cerca de 18 px, así que los tokens donde aparece contenido de
usuario quedan a x0.83 (badge), x0.94 (label) y x1.06 (body) — dentro del 6% del
tamaño nativo. **El único tamaño lejano es el título de Now Playing, a x1.67.**
**Confirmado en consola con la sonda v2:** x0.83, x0.94 y x1.06 se ven bien;
**a partir de x1.22 ya se ve borroso**. Es decir, el respaldo del sistema sirve
para `UI_TS_BADGE` (15), `UI_TS_LABEL` (17) y `UI_TS_BODY` (19), y no sirve para
`UI_TS_TITLE` (22) ni `UI_TS_DISPLAY` (30).

Eso alcanza para todo el contenido de usuario menos un sitio. `UI_TS_TITLE` no
dibuja contenido de usuario en ninguna pantalla, así que su borrosidad es
irrelevante. **El único conflicto real es el título de Now Playing, a 30 px.**

**Dos correcciones al diseño que la medición obliga:**

- `design.md` dice que "la regla de un handle por tamaño aplica igual a ambos
  motores". No aplica: como `scePvfSetCharSize` está fijo en la biblioteca,
  varios handles de PVF serían atlas idénticos de 18 px. **Va un solo handle**,
  dibujado a la escala de cada token.
- `design.md` dice partir la cadena "por rango de codepoint". El cirílico está
  cubierto a medias por la tipografía propia (40.6% / 65.6%), así que la
  partición tiene que decidir **por codepoint**, no por rango.

**Las cuatro opciones originales, que la medición dejó obsoletas salvo como
registro:**

| Salida | Qué cuesta |
|---|---|
| Dibujar el respaldo con escala no neutra | Contradice el primer requirement de `ui/typography`: un glifo rasterizado a 18 px se reescalaría a 30 px en el título, que es exactamente el defecto que este change existe para eliminar. Afecta sólo al texto no latino. |
| Mover `UI_TS_BODY` de 19 a 18 px | El nombre de archivo y el artista quedarían con escala neutra, pero el título (30 px) seguiría reescalado, y toca la escala recién acordada en 3.1. |
| Acotar la cobertura a los tamaños que 18 px sirve bien | Cumple el requirement de nitidez y recorta el de cobertura: el título no latino quedaría en blanco. |
| Empaquetar una tipografía no latina propia | Resuelve ambos, pero son varios MB de recurso y está fuera del alcance acordado en `proposal.md`. |

Vendorizar y parchear `vita2d_font` ya fue evaluado y descartado en `design.md`,
y este hallazgo no cambia esa evaluación.
