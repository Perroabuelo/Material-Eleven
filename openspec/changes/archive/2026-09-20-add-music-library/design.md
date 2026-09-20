## Context

Ver `proposal.md` — Why para la motivación, y los specs `library/index` y `library/views` para los requirements.

Lo que condiciona el diseño es cómo está hecha hoy la aplicación:

- **Un solo hilo de interfaz.** Toda la aplicación es un lazo de fotogramas por pantalla (`Menu_DisplayFiles`, `Menu_RunNowPlayingLoop`, `Menu_DisplaySettings`). No hay hilos de trabajo propios; los únicos hilos son los internos de la biblioteca de audio. Un escaneo que tarde minutos no puede ser una llamada bloqueante: se llevaría por delante el dibujado, la entrada y la posibilidad de abandonarlo.
- **Existe precedente de lazo anidado.** `Menu_PromptFilter` (`source/menus/menu_displayfiles.c`) ya corre un lazo de fotogramas propio mientras el teclado del sistema está delante, dibujando, leyendo el pad y con salidas acotadas. El escaneo puede usar la misma forma.
- **La cola es la carpeta.** `Menu_GetMusicList` (`source/menus/menu_audioplayer.c:34`) relee `cwd` en cada `Menu_PlayAudio` y llena un `playlist[1024][512]` estático. No hay ninguna otra fuente posible de cola.
- **Los metadatos se leen al abrir el decoder.** MP3, OGG y OPUS extraen los tags como parte de abrir el archivo para reproducirlo; FLAC tiene lectura de tags independiente de la ruta (`FLAC__metadata_get_tags`), y los formatos de tracker tienen `xmp_test_module` (`libs/include/xmp.h:319`), que lee solo la cabecera.
- **Hay dos archivos de estado en disco, ambos triviales.** `ux0:data/ElevenMPV/config.cfg`, escrito y leído con `snprintf`/`sscanf` sobre una plantilla de texto, y `lastdir.txt`, una sola línea. El código ya sabe escribir y leer archivos pequeños (`source/fs.c`).
- **`menu_audioplayer.c` es el archivo más frágil del repositorio** según el índice del propio proyecto (peor salud, 1.0/10). El rediseño de la cola lo toca.

## Goals / Non-Goals

**Goals:**

- Que el escaneo pueda abandonarse en cualquier momento y que la reproducción en curso no se entere de que ocurrió.
- Que el índice sea reconstruible siempre, de modo que su formato no sea un compromiso a largo plazo.
- Que la fase 1 deje una biblioteca utilizable aunque las fases 2 y 3 nunca lleguen.
- Que el camino del navegador de carpetas salga de este change con el mismo comportamiento observable con el que entró.

**Non-Goals:**

- Escaneo en segundo plano mientras el usuario usa otras pantallas. El escaneo es una operación en primer plano, con su pantalla y su progreso.
- Detección automática de cambios en disco. El reescaneo es manual, por decisión del usuario.
- Índice incremental que compare fechas para reescanear solo lo nuevo. El reescaneo rehace el recorrido completo; lo que se reaprovecha son los tags y las carátulas ya leídos.

## Decisions

### El índice es dato derivado, nunca fuente de verdad

La fuente de verdad son los archivos en disco. El índice es una caché de lo que se encontró en ellos, y siempre puede reconstruirse reescaneando.

La consecuencia práctica es que **el formato del índice no es un compromiso**: lleva un número de versión, y una versión que no se reconoce no se migra, se descarta y se ofrece reescanear. Es exactamente la mecánica que `config.c` ya usa con `CONFIG_VERSION`, pero aquí sin costo para el usuario, porque no hay preferencias que perder.

Eso permite elegir para la fase 1 el formato más simple y cambiarlo después si medir lo justifica.

**Formato elegido:** texto, una pista por línea, con una cabecera que lleva versión, carpeta escaneada y número de pistas. Alternativa considerada: registros binarios de tamaño fijo, que se cargarían sin parsear y permitirían lectura aleatoria. Se descarta para la fase 1 porque el texto se inspecciona desde el PC cuando algo va mal, coincide con cómo el proyecto ya guarda su estado, y el costo de parsear unos miles de líneas una vez al arrancar no es el cuello de botella de esta feature. Si el arranque resulta lento con colecciones grandes, cambiar a binario es una decisión local que no toca specs.

### La ruta absoluta es la identidad de una pista

No la posición en el índice, que cambia en cada reescaneo. Todo lo que tenga que sobrevivir a un reescaneo —la caché de carátulas, y mañana las playlists— se referencia por ruta.

### "Desconocido" es presentación, no dato

El índice guarda el artista y el álbum vacíos cuando no los hay; la etiqueta "Desconocido" y su posición al final de la lista las pone la vista.

Dos razones: un artista que de verdad se llame "Desconocido" no se mezcla con el cubo, y el índice no queda escrito en un idioma. Alternativa considerada: escribir la cadena en el índice al escanear — se descarta por ambas razones.

### El escaneo avanza por trozos dentro de un lazo de fotogramas, no en un hilo

Cada fotograma procesa un número acotado de entradas de directorio y luego dibuja progreso y lee el pad. El escaneo termina cuando se vacía la pila de carpetas pendientes o cuando el usuario lo abandona.

Alternativa considerada: un hilo de trabajo (`sceKernelCreateThread`), que dejaría la interfaz completamente fluida. Se descarta para este change porque obliga a sincronizar el acceso al índice entre el hilo que lo llena y el que lo dibuja, y esa concurrencia sería lo único de este change sin precedente en el repositorio. El troceado da progreso y abandono sin sincronización ninguna, y sigue la forma que `Menu_PromptFilter` ya estableció.

El tamaño del trozo se ajusta para que un fotograma no se alargue de forma perceptible; es una constante, no una decisión de arquitectura.

### El recorrido usa una pila explícita de carpetas pendientes, no recursión

Un árbol de carpetas elegido por el usuario puede ser arbitrariamente profundo, y la pila de un hilo en esta consola es limitada. Una función recursiva por carpeta convierte una estructura de carpetas rara en una caída. La pila explícita, además, es lo que permite que el escaneo se suspenda entre fotogramas: su estado es la pila.

La profundidad se acota igualmente, y alcanzar el tope deja esa rama sin explorar en vez de caerse.

### El escaneo no reserva un arreglo de entradas de directorio

`sceIoDread` se llama entrada por entrada y cada entrada se decide y se descarta en el acto: si es carpeta, se apila; si es audio reconocido, se anota en el índice; si no, se olvida. Nunca hay más de una `SceIoDirent` viva.

Esto elimina de la ruta del escáner el `calloc(MAX_FILES, sizeof(SceIoDirent))` —unos 360 KB— que hoy hace cada lectura de directorio, y elimina el techo por carpeta. El techo de la biblioteca pasa a ser un presupuesto de memoria declarado.

`MAX_FILES` se queda donde está, como límite del navegador de carpetas, y su bucle de lectura pasa a respetarlo, que es lo que hoy no hace.

### El escaneo tiene dos pasadas, y la segunda es reanudable

La primera pasada recorre y anota rutas, tamaños y fechas, y persiste el índice. Al terminar, la biblioteca **ya es utilizable**: hay lista de canciones y hay "Recientes".

La segunda pasada recorre el índice leyendo tags, persistiendo cada cierto número de pistas. Si se abandona o se corta la energía, lo leído hasta ahí queda guardado y la siguiente pasada continúa por las pistas que aún no tienen tags.

Esto es lo que hace que las fases 1 y 2 sean entregas separables de verdad y no una división en el papel: la fase 1 es la primera pasada, la fase 2 es la segunda.

### La carpeta de escaneo se elige reutilizando el navegador de carpetas

En vez de escribir un selector de carpetas nuevo, el navegador existente se abre en un modo donde la acción de confirmar elige la carpeta actual en lugar de reproducir. Es la misma pantalla, la misma navegación y los mismos controles que el usuario ya conoce.

Alternativa considerada: un selector propio — se descarta por duplicar navegación, dibujado y manejo de entrada ya resueltos.

### El mini reproductor se comparte, no se escribe dos veces

Es la misma decisión que la de elegir carpeta reutilizando el navegador. Hoy `Menu_DrawMiniPlayer` y `Menu_HandleMiniPlayerTouch` viven privados en `menu_displayfiles.c`; pasan a un sitio común y la biblioteca los llama. Un segundo mini reproductor no daría nada y sería una segunda cosa que mantener alineada con el transporte.

### La carpeta de escaneo vive en su propio archivo, no en `config.cfg`

`Config_Load` descarta el archivo entero cuando `CONFIG_VERSION` sube, y con él todas las preferencias del usuario (`source/config.c`). Añadir un campo al config costaría, a todo el que actualice, sus ajustes de ecualizador, orden y dispositivo.

Un archivo propio junto a `lastdir.txt` —que ya es exactamente eso— evita el problema por completo.

### La cola se convierte en una estructura con dueño, alimentada por dos productores

La cola pasa a ser un módulo con su lista de rutas y su posición actual, y se llena desde fuera. El productor de carpeta es el `Menu_GetMusicList` de hoy, movido sin cambiarle el comportamiento; el productor de biblioteca vuelca las pistas de la vista vigente en el orden en que se ven.

Dos restricciones sobre cómo se implanta, porque toca el archivo más frágil del repositorio:

1. La extracción de la cola se entrega **en su propio commit, sin cambio de comportamiento observable**: antes y después, reproducir desde una carpeta da exactamente la misma cola, el mismo orden, el mismo aleatorio y la misma repetición. Ese commit no añade la biblioteca.
2. El `playlist[1024][512]` estático se reemplaza por memoria dimensionada a la cola real. Los 512 KB que hoy están reservados de forma permanente dejan de estarlo.

### Las carátulas se cachean por álbum, como miniaturas crudas

La clave de caché es el álbum, no la pista: un disco de doce canciones lleva doce copias de la misma imagen, y extraerla una vez reduce el trabajo en un orden de magnitud. Las pistas sin álbum no participan de la caché por álbum y se tratan individualmente.

Las miniaturas se guardan como píxeles crudos a tamaño fijo. Alternativa considerada: guardarlas como PNG o JPEG, que ocuparían mucho menos disco — se descarta porque no existe ningún codificador de imagen en el proyecto y habría que incorporar uno; el crudo no necesita ni codificador ni parseo de cabecera, y el disco es el recurso que sobra en esta consola.

La extracción reutiliza el código que ya existe por formato (`source/audio/mp3.c:156`, `source/audio/flac.c:138`, `source/audio/opus.c:56`); OGG no lo tiene y sus pistas quedan sin carátula hasta que alguien lo escriba.

### La fase 3 tuvo una medición como puerta, y la pasó

La puerta era esta: medir sobre una colección real el costo de solo recorrer, el
de recorrer leyendo tags y el de extraer y decodificar una carátula, y averiguar
si `vita2d_load_JPEG_buffer` usa el decodificador JPEG por hardware de la consola
o una ruta por software. Si los números la descartaban, la fase se retiraba del
change y sus requirements salían de `library/index` y `library/views` antes de
archivarlo.

**Se midió, y la fase 3 se escribe.** El grupo 7 de `tasks.md` se queda.

Medido en consola sobre `ux0:Mis cosas/`: 291 pistas reproducibles —237 FLAC y 54
MP3— repartidas en 49 carpetas, 481 entradas de directorio, 5959 MB, profundidad
2. Las FLAC son de 96 kHz y 24 bits y pesan 20 MB de media, así que esta
colección es bastante más pesada que la típica y sus números valen como techo y
no como caso medio. El informe crudo quedó en `ux0:data/elevenmpv/measure.txt`,
producido por un spike que no entra en el árbol.

| pasada | total | por pista |
|---|---|---|
| A — recorrer con `sceIoDread`, sin abrir ningún archivo | 0,286 s | 0,98 ms |
| B — leer tags, añadido sobre A | 46,352 s | 159 ms |
| C — carátulas, una vez por álbum | 7,995 s | — |

B por formato, en microsegundos por pista: FLAC 187.110 (237 pistas), MP3 37.172
(54), OGG 55.807, OPUS 72.453, tracker 9.273 con `xmp_test_module`, WAV 478. OGG,
OPUS, WAV y tracker se midieron sobre un corpus de apoyo, porque esta colección no
tiene ninguno de los cuatro.

Tres conclusiones, y la primera invierte lo que el proposal daba por supuesto:

1. **El riesgo no estaba en la fase 3 sino en la 2.** Recorrer es gratis: 0,29 s
   por la colección entera. Leer tags cuesta 161 veces más que recorrer, y las
   carátulas solo añaden un 17% sobre eso. FLAC se lleva el 96% de la pasada de
   tags —44,3 s de 46,4— a 187 ms por pista, cinco veces lo que cuesta un MP3.
2. **Ocho segundos no justifican retirar una fase.** Es el costo de las carátulas
   de una colección entera, pagado una sola vez y guardado en disco.
3. **Cachear por álbum y no por pista queda confirmado medido**: 32,5 s haciéndolo
   por pista frente a 8,0 s haciéndolo por álbum, 4,1 veces menos.

`vita2d_load_JPEG_buffer` decodifica **por software**. Los símbolos indefinidos de
`vita2d_image_jpeg.o` dentro de `libvita2d.a` son libjpeg puro —
`jpeg_CreateDecompress`, `jpeg_read_header`, `jpeg_read_scanlines`,
`jpeg_finish_decompress`— y no hay una sola referencia a `sceJpeg*` en toda la
biblioteca. Decodificar una carátula de 600x600 cuesta 93 ms de CPU, y ese costo
no depende del tamaño de la miniatura, porque libjpeg decodifica a tamaño completo
antes de que nadie pueda reescalar. Es una razón más para pagarlo una vez por
álbum y guardar el resultado.

Por qué FLAC sale tan caro no se midió; solo se midió que lo es. La hipótesis es
el patrón de E/S y no el parseo: `FLAC__metadata_get_tags` va por `FILE*` con el
buffer por defecto, y este repositorio ya se encontró antes con lo mismo —
`source/audio/flac.c` lleva un buffer de lectura anticipada de 128 KB puesto
justamente porque las llamadas por defecto de dr_flac emitían muchos `sceIoRead`
pequeños. Si la pasada de tags resulta incómoda de esperar, ahí hay margen que
probar antes de tocar el diseño.

### El techo son 4000 pistas, y la miniatura 128x128 RGBA

Las dos preguntas que el grupo 0 tenía que cerrar, cerradas con sus números.

**4000 pistas.** Un registro del índice mide unos 470 bytes: ruta 256, título,
artista y álbum 64 cada uno, y tamaño, fecha y extensión. 4000 pistas son ~1,9 MB
en RAM, frente a los 512 KB que `playlist[1024][512]` ya reserva hoy de forma
permanente y sin condiciones. La ruta medida de media son 78 bytes, muy por debajo
del cap. Lo que fija el número no es la memoria sino la espera: a 159 ms por pista,
4000 pistas son unos 11 minutos de pasada de tags. Es soportable precisamente
porque esa es la *segunda* pasada, reanudable y abandonable, y porque la primera
—que es la que deja la biblioteca utilizable— tarda 4 segundos para esas mismas
4000 pistas. El techo es una constante; subirlo una vez medido el consumo real de
un índice lleno es un cambio de una línea.

**128x128 RGBA, 64 KB por álbum.** El tamaño no afecta al costo de decodificar,
que es fijo; solo a disco y a RAM. 128 px cubre a 2x las filas de lista, que miden
64 px, y aguanta una rejilla de álbumes sin verse blanda. Con la proporción medida
—44 carátulas para 291 pistas— un índice lleno de 4000 pistas serían unas 600
carátulas: 38 MB en disco, que es el recurso que sobra en esta consola.

## Risks / Trade-offs

- **El costo del escaneo con tags era la incógnita, y resultó ser el costo dominante**: 159 ms por pista, 161 veces lo que cuesta recorrer, con FLAC llevándose el 96%. Medido, el diseño en dos pasadas deja de ser una precaución y pasa a ser el motivo por el que esto es usable: la primera pasada termina en segundos y entrega biblioteca, y la segunda es reanudable e interrumpible.
- **La fase 3 se temía inviable y resultó ser lo barato**: 8 s por una colección entera, un 17% sobre la pasada de tags, pagados una vez y cacheados. Sigue aislada al final y ninguna decisión de las fases 1 y 2 depende de ella, pero ya no es el riesgo del change.
- **El rediseño de la cola toca el archivo con peor salud del repositorio** → va en un commit propio, sin cambio de comportamiento, verificable comparando contra el build anterior: misma carpeta, misma cola. La biblioteca se conecta después.
- **Un escaneo largo puede parecer colgado** → progreso visible con cuenta de pistas encontradas, y abandono siempre disponible, verificado también en el caso en que el usuario abandone a mitad.
- **El índice puede quedar desfasado respecto al disco** entre reescaneos: el usuario borra un archivo desde el PC y la pista sigue listada → abrir una pista cuya ruta ya no existe falla con aviso y sin caída, y sugiere reescanear. No se intenta validar el índice entero al arrancar, porque eso es un escaneo.
- **Elegir la raíz de un dispositivo como carpeta de escaneo** haría recorrer `ux0:app`, `ux0:patch` y demás, lentísimo y sin música → la profundidad acotada y el abandono lo hacen soportable, y el usuario ve el progreso y puede cancelar. No se filtran carpetas por nombre: adivinar qué carpetas del usuario no valen la pena es peor que dejarle cancelar.
- **Colecciones por encima del techo** → truncado declarado y visible, en vez de un límite silencioso. El techo es una constante, y subirlo es un cambio de una línea una vez medido el consumo real.

## Migration Plan

No hay datos previos que migrar: la biblioteca no existe. Un usuario que actualice ve una entrada nueva en el nav rail y una biblioteca sin carpeta elegida.

La entrega es **en commits por partes**, nunca un commit único. Cada commit deja el árbol compilable y la aplicación utilizable, para que un fallo sea atribuible por bisección. El orden y los cortes están en `tasks.md`; el esqueleto es:

```
  0  medicion          spike, sin codigo de produccion
  1  acotar lecturas   arreglo del desbordamiento, aislado
  2  cola con dueno    refactor sin cambio de comportamiento
  --- a partir de aqui la biblioteca es visible ---
  3  fase 1            recorrido, indice, persistencia, pantalla
  4  fase 2            tags y vistas por artista y album
  5  fase 3            caratulas         (condicionada al paso 0)
```

Los commits 1 y 2 tienen valor por sí solos y podrían quedarse aunque el resto se abandonara. La vuelta atrás de los commits 3 en adelante es retirar la entrada del rail: el índice es un archivo en `ux0:data/ElevenMPV/` que puede borrarse sin afectar a nada más.

## Open Questions

- ~~**Techo de pistas de la biblioteca.**~~ Cerrada por el grupo 0: 4000 pistas. No cambia specs ni tareas — el spec ya exigía que existiera un techo, que el truncado se informe y que no se desborde.
- ~~**Tamaño de las miniaturas.**~~ Cerrada por el grupo 0: 128x128 RGBA, 64 KB por álbum.
- **Orden de "Recientes"** cuando varios archivos comparten fecha de modificación al segundo, cosa habitual en una copia masiva desde el PC. Criterio de desempate a elegir al implementar.
