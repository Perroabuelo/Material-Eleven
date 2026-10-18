## Context

Ver `proposal.md` para la motivación, y `specs/` para el comportamiento.

Estado actual que condiciona el diseño:

- **Retroceder tiene tres caminos.** En Reproduciendo, L (`Menu_RunNowPlayingLoop`) y el botón de anterior en pantalla (`Menu_HandleTransportTouch`) llaman directo a `Music_HandleNext(SCE_FALSE, SCE_FALSE)`. El mini reproductor llama a `Music_Previous()`, que hace lo mismo. Ninguno mira la posición. `TrackAdvance_Run` (`track_advance.c`, puro) decide qué pista se abre.
- **Pausa.** `Audio_Pause()` invierte `paused` en el acto, y `Audio_Decode` (hilo de audio) escribe silencio mientras `paused` está puesto. La barra de progreso (`Menu_HandleSeekTouch`) usa esa misma función para pausar alrededor de `Audio_Seek` y después restaurar el estado. La llaman además el botón de confirmar, el botón de reproducir en pantalla y `Music_TogglePlayPause` (mini reproductor). El hilo de `power_tick_thread` (`utils.c`) y la leyenda leen `Audio_IsPaused()`.
- **Hilo de audio.** Un canal (`vitaaudiolib.c`). El hilo se recrea en cada pista (`Audio_Init` → `vitaAudioInit`), y `Audio_Term` pone `paused` en falso. El buffer que llena `Audio_Decode` es de `grain` frames (960 por defecto, unos 20 ms) por los canales de la pista, y la frecuencia es la de la pista. La toma del visualizador (`VizTap_Write`) copia ese mismo buffer después de llenarlo.
- **Un punto por fotograma en cualquier pantalla.** `Utils_ReadControls` llama a `Music_Update()` al principio de cada fotograma, en todas las pantallas, el diálogo de búsqueda y los escaneos, también con la pantalla apagada (el lazo sigue a ritmo normal, visto en la consola en `add-visualizer`). `Music_Update` detecta el fin de pista (`playing == SCE_FALSE`) y llama a `Music_HandleNext(SCE_TRUE, repeat)`.
- **Botones de Reproduciendo.** Confirmar pausa, volver regresa, L y R cambian de pista, Triángulo baraja, Cuadrado repite, las flechas manejan las vistas y los estilos, y START apaga la pantalla. **SELECT está libre en Reproduciendo**: solo lo usa el panel de captura del overlay de debug (`UI_Capture_HandleButtons`, cuando `debug_mode == UI_DEBUG_CAPTURE`), que en las listas convive con la acción de la lista. La leyenda ya tiene un chip de SELECT (`HINT_BTN_SELECT`, usado en Carpetas).
- **Menú flotante existente.** La Biblioteca dibuja su menú de opciones como un rectángulo redondeado abajo a la derecha (`Menu_DrawLibraryOptions`, `OPTIONS_ITEM_H`, `UI_DrawRowHighlight`), lo maneja con `Utils_ListStep`, confirmar y volver, y cierra con un toque fuera.
- **Barra de estado.** `StatusBar_Display` dibuja la hora y la batería alineadas a la derecha. La llaman Reproduciendo, Carpetas y Biblioteca. Ajustes no la muestra.
- **`config.cfg`** está en la versión 4. Este cambio no la toca. Los cambios apilados que vienen después toman las versiones 5 en adelante.

## Goals / Non-Goals

**Goals:**

- Que la regla de los 3 segundos, la cuenta del temporizador y la rampa del fundido sean código puro sin vita2d ni SCE, probado en PC con ASan y UBSan.
- Un solo camino de retroceder para las tres superficies, para que no puedan divergir.
- Que el hilo de audio no tome locks, no reserve memoria y no haga llamadas al sistema nuevas: solo multiplica muestras por una ganancia.
- Que la barra de progreso siga buscando sin fundidos y sin cambiar el estado de pausa.

**Non-Goals:**

- Fundidos al cambiar de pista o al buscar (son de `audio-mixer`).
- Un mezclador o un puerto de audio fijo. Este cambio trabaja sobre el hilo por pista de hoy.
- Calibrar la curva del fundido por oído. Es lineal en amplitud, que para 100 ms basta.

## Decisions

### D1. Retroceder: una sola función y la regla pura en `track_advance`

`track_advance.h` gana `TrackAdvance_PrevRestarts(uint32_t position_ms)`, que devuelve 1 si `position_ms > TRACK_ADVANCE_RESTART_MS` (3000). Se prueba en PC con 0, 3000, 3001 y la posición máxima.

`Music_Previous()` pasa a ser el único camino de retroceder. L en Reproduciendo y el botón de anterior en pantalla dejan de llamar a `Music_HandleNext` directo y llaman a `Music_Previous()`, igual que el mini reproductor. `Music_Previous()`:

1. Si no hay pista o la cola está vacía, no hace nada, como hoy.
2. Si `TrackAdvance_PrevRestarts(Audio_GetPositionMs())`, reinicia: `Music_SeekTo(0.0)` (D3), registra `track` `event=restart` y, si la vista de letras sigue la canción, la lleva a la primera línea (`Menu_LyricsSnap`, el mismo ajuste que hace una búsqueda).
3. Si no, `Music_HandleNext(SCE_FALSE, SCE_FALSE)`, como hoy.

`Audio_GetPositionMs` es la posición del decoder, que va unos dos grains (unos 40 ms) por delante de lo que se oye. Para un umbral de 3 s no importa.

*Alternativa:* reiniciar reabriendo la pista (`Music_HandleNext(…, replay = SCE_TRUE)`, lo que hace la repetición). Funciona con cualquier formato, pero vuelve a decodificar la carátula, a cargar las letras y a armar la leyenda, con el tirón de un cambio de pista. Buscar a 0 es inmediato y deja todo en su lugar. Todos los decoders implementan `seek(0.0)`: XM usa `xmp_seek_time(0)`, y los demás buscan al sample 0. Se verifica en la consola con un archivo de cada formato (tarea 5.x).

### D2. Fundido de pausa: la pausa del usuario es una intención que el hilo de audio cumple

Módulo puro nuevo `source/fade.c` / `include/fade.h`:

```c
typedef struct {
	float gain;     // 0..1, la ganancia del último frame aplicado
	float target;   // 0 o 1
	float step;     // cuánto se mueve gain por frame
} Fade_Ramp;

void Fade_Start(Fade_Ramp *r, float target, unsigned int frames);  // llegar a target en `frames`
void Fade_Apply(Fade_Ramp *r, int16_t *buf, unsigned int frames, unsigned int channels, float master_from, float master_to);
int  Fade_Done(const Fade_Ramp *r);                                 // gain == target
```

`Fade_Apply` multiplica cada frame por `gain × master`, donde `gain` avanza `step` por frame hacia `target` sin pasarse y `master` se interpola linealmente de `master_from` a `master_to` a lo largo del buffer (D4). Redondea y satura a `int16`. Con ganancia 1 y master 1 no toca el buffer (atajo), así que el camino normal no gasta nada.

En `audio.c`:

- `static volatile int pause_request` (0 o 1) lo escribe solo el hilo de UI. `paused` (lo que suena) pasa a escribirlo solo el hilo de audio.
- `Audio_TogglePause()` (nueva, la que usan el botón de confirmar, el botón de reproducir en pantalla y `Music_TogglePlayPause`) invierte `pause_request`.
- `Audio_IsPaused()` devuelve `pause_request`: la intención. Así la pantalla, la leyenda y el log cambian en el fotograma de la pulsación, y `power_tick_thread` deja de mantener despierta la consola en cuanto se pide la pausa.
- `Audio_Decode`, al principio de cada callback, compara `pause_request` con el objetivo de la rampa. Si cambió, `Fade_Start(&ramp, pause_request ? 0 : 1, frames_de_100_ms)` desde la ganancia actual, lo que también invierte un fundido a medias sin saltos. Si `paused` está puesto y se pidió reanudar, lo quita y decodifica. Después de decodificar aplica la rampa, y si la rampa llegó a 0 con `pause_request` puesto, pone `paused`. Desde el callback siguiente escribe silencio, como hoy.
- `frames_de_100_ms = rate / 10`, con la frecuencia de la pista que el callback ya conoce (`viz_rate`).
- `Audio_Term` y `Audio_Init` reinician `pause_request = 0`, `paused = 0` y la rampa en ganancia 1, igual que hoy reinician `paused`.

*Alternativas:* bajar el volumen del puerto con `sceAudioOutSetVolume` en pasos desde la UI. Tiene la resolución del fotograma (16 ms) y una llamada al sistema por paso, y el volumen del puerto ya lo usa `vitaAudioSetVolume`. Aplicar la rampa en `vitaAudioChannelThread` mezcla la lógica con la librería heredada, como se descartó en `add-visualizer` (D1).

### D3. Buscar sin fundido: `Audio_Hold`

La barra de progreso necesita silencio inmediato mientras busca, sin tocar la intención del usuario. `audio.c` gana `Audio_Hold(SceBool on)`: con `hold` puesto, `Audio_Decode` escribe silencio sin decodificar y sin mover la rampa. `Music_SeekTo(double fraction)` (nueva, en `menu_audioplayer.c`) hace `Audio_Hold(1)`, `Audio_Seek(fraction)`, `Audio_Hold(0)`, y la usan `Menu_HandleSeekTouch` y el reinicio de D1. La función vieja `Audio_Pause()` se borra: después de esto nadie la necesita, y dejarla invitaría a pausar sin fundido.

Con la música en pausa, `hold` no cambia nada (ya se escribe silencio). Con la música sonando, el corte de la búsqueda es el mismo de hoy. Si la búsqueda llega en medio de un fundido de pausa, la rampa queda congelada mientras dura `hold` y sigue después.

### D4. Temporizador: módulo puro, avanzado una vez por fotograma

Módulo puro nuevo `source/sleep_timer.c` / `include/sleep_timer.h`:

```c
typedef enum { SLEEP_OFF, SLEEP_15, SLEEP_30, SLEEP_45, SLEEP_60, SLEEP_90, SLEEP_END_OF_TRACK, SLEEP_OPTION_COUNT } Sleep_Option;

typedef struct { Sleep_Option option; uint64_t deadline_us; } Sleep_Timer;

typedef enum { SLEEP_TICK_NONE, SLEEP_TICK_EXPIRED } Sleep_Tick;

void        SleepTimer_Set(Sleep_Timer *t, Sleep_Option o, uint64_t now_us);
Sleep_Tick  SleepTimer_Tick(Sleep_Timer *t, uint64_t now_us, float *gain);   // gain: 1, o la rampa de los últimos 10 s
int         SleepTimer_TrackEnded(Sleep_Timer *t);                           // 1 si estaba en fin de pista (y lo apaga)
uint32_t    SleepTimer_MinutesLeft(const Sleep_Timer *t, uint64_t now_us);   // redondeado hacia arriba, 0 si no hay tiempo
const char *SleepTimer_Name(Sleep_Option o);                                 // "off", "15", ..., "track" (para el log)
```

`SleepTimer_Tick` devuelve `gain = (deadline − now) / 10 s` dentro de los últimos 10 segundos, 1 antes, y `SLEEP_TICK_EXPIRED` una sola vez al llegar al vencimiento, cuando deja la opción en `SLEEP_OFF`. Se prueba en PC con un reloj falso, incluido el redondeo de minutos (15:00 → 15, 14:59 → 15, 0:20 → 1), un salto de reloj que pasa de largo el vencimiento (vence una vez, ganancia 0), y reemplazar o cancelar a mitad del fundido final.

El reloj es `sceKernelGetProcessTimeWide()`, el mismo que usa `KeyRepeat_Update`.

**Integración** (`menu_audioplayer.c`, junto a `Music_Update`, llamada desde `Utils_ReadControls`): `Music_SleepTick()` cada fotograma, en cualquier pantalla:

1. `SleepTimer_Tick` da la ganancia, que se pasa a `Audio_SetMasterGain(gain)`. Es un `float` alineado que escribe la UI y lee el hilo de audio (una escritura de 32 bits es atómica en el Cortex-A9). `Audio_Decode` recuerda el master del buffer anterior e interpola hasta el nuevo (`master_from`, `master_to` en D2), así que el paso por fotograma no se oye. El master es global, no de la pista: `Audio_Term` no lo toca, y por eso el fundido sigue en la pista nueva si cambia a mitad.
2. Al vencer con la música sonando: registra `sleep` `event=expire`, pide la pausa (`Audio_TogglePause()` si `!Audio_IsPaused()`) y deja el master en 0 hasta que `Audio_IsSilent()` (nueva: `paused` del hilo de audio puesto) diga que la pausa se cumplió. Recién entonces lo vuelve a 1. Si lo volviera a 1 en el acto, el master subiría a lo largo de un buffer mientras la rampa de pausa recién empieza a bajar, y se oiría un golpe.
3. Al vencer en pausa: solo registra el vencimiento. El master ya está en 0 y vuelve a 1 en el mismo fotograma, porque no suena nada.
4. Cancelar o reemplazar durante el fundido final: el master vuelve a 1 en el fotograma siguiente, interpolado a lo largo de un buffer (unos 20 ms). Es una subida rápida, sin el golpe de un salto en una sola muestra.

**Fin de pista:** en `Music_Update`, cuando la pista termina sola, `SleepTimer_TrackEnded()` dice si estaba el modo de fin de pista. Si es así, se registra `sleep` `event=expire mode=track`, se llama a `Audio_SetStartPaused()` y después a `Music_HandleNext(SCE_TRUE, repeat)` como siempre. `Audio_SetStartPaused()` deja una marca que `Audio_Init` consume antes de `vitaAudioSetChannelCallback`: pone `pause_request = 1` y `paused = 1`, de modo que el primer callback de la pista nueva ya escribe silencio y no se escapa ni un buffer. `Music_HandleNext` borra la marca al terminar, abra o no una pista, para que una marca que no se usó (ninguna pista abrió) no pause la próxima reproducción que elija el usuario.

*Alternativas:* contar el tiempo en un hilo propio. Sería otro recurso que crear y liberar, y `Utils_ReadControls` ya corre en todas las pantallas y con la pantalla apagada. Bajar el volumen con `sceAudioOutSetVolume` para el fundido de 10 s: descartado por lo mismo que en D2. Para el modo de fin de pista, quedarse en la pista que terminó en vez de pasar a la siguiente: al reanudar volvería a empezar la misma pista, y lo natural al despertar es seguir con lo que venía.

### D5. El menú del temporizador

Estado de pantalla en `menu_audioplayer.c`: `static SceBool sleep_menu_open` y `static int sleep_menu_index`. Se dibuja con la misma forma que el menú de opciones de la Biblioteca (rectángulo redondeado `UI_COLOR_SURFACE_2`, resaltado con `UI_DrawRowHighlight`, `OPTIONS_PAD` de 8 px), con un título arriba y una marca de la opción vigente dibujada como geometría. Las filas de la Biblioteca miden 56 px (`OPTIONS_ITEM_H`, local de `menu_library.c`), y siete filas de 56 más el título no entran entre la barra de estado y la leyenda. Este menú usa filas propias de 44 px, que siguen por encima del tamaño táctil mínimo de `ui/rendering`, y sus constantes quedan locales de `menu_audioplayer.c`. Va anclado abajo a la derecha, sobre la leyenda, y se dibuja después de la vista, en las tres vistas. No se extrae un componente genérico de menú: son dos usos, y la Biblioteca tiene además un selector y una confirmación con su propia lógica.

Manejo, antes del resto del input de Reproduciendo: si `sleep_menu_open`, se atienden `Utils_ListStep`, confirmar (elige, `SleepTimer_Set`, registra `sleep` `event=set`, cierra), volver o SELECT (cierra) y el toque (una fila elige, fuera cierra), y se sale del manejo del fotograma sin mirar los demás botones ni los toques de la vista. SELECT abre el menú con `sleep_menu_index` en la opción vigente. Al salir de Reproduciendo (volver, nav rail, una pista que falla) el menú se cierra, para que no aparezca abierto al volver.

Con el panel de captura del overlay de debug activo, SELECT hace las dos cosas, como en las listas: guarda la captura y abre o cierra el menú. Es una herramienta de desarrollo y no merece un caso especial.

### D6. Indicador en la barra de estado

`StatusBar_Display` dibuja, a la izquierda de la batería, una luna creciente dibujada como geometría (un círculo del color del texto y otro del color del fondo, desplazado, encima), con el borde suavizado que pide `ui/rendering` y el texto con `UI_FACE_MONO`, `UI_TS_BADGE`, en `UI_COLOR_TEXT_TERTIARY`, igual que la hora. El texto es `"%u min"` con `SleepTimer_MinutesLeft`, o `STR_SLEEP_STATUS_TRACK` ("Fin de pista" / "End of track"). La barra de estado lee el temporizador con una función de solo lectura que expone `menu_audioplayer.c` (`Music_SleepStatus`), para no repartir el estado.

### D7. Leyenda y textos

La leyenda de Reproduciendo agrega `{ { HINT_BTN_SELECT }, 0, Lang_Get(STR_HINT_SLEEP_TIMER) }` entre Cuadrado y START. Con el menú abierto, la leyenda es solo `{ CONFIRM, STR_HINT_SELECT }` ("Select" / "Seleccionar", que ya existe) y `{ CANCEL, STR_HINT_CLOSE }` (nueva: "Close" / "Cerrar").

**La leyenda no tenía lugar.** Medida en la consola y después con la fuente (IBM Plex Mono, 0,6 em por carácter: 10,2 px a 17 px y 9 px en los chips a 15 px, sobre 840 px útiles), ya venía llena antes de este cambio: en inglés Repeat no se veía. La entrada de SELECT necesita unos 135 px (el chip "SELECT" mide 66), y acortar solo su texto no alcanza. Para que entre, la leyenda se compacta:

- L y R dicen "<< >>" en los dos idiomas (antes "Previous / Next" / "Pista").
- D-pad arriba y abajo van en una sola entrada con los dos chips y "Views" / "Vistas" (antes dos entradas, "Lyrics" / "Letras" y "Visualizer" / "Visualizador").
- Triángulo dice "Mezclar" en español (antes "Aleatorio"); en inglés sigue "Shuffle".
- La entrada de SELECT dice "Sleep" / "Dormir".

El orden queda: confirmar, volver, L R, ▲▼, ◀▶ Estilo (solo en el visualizador), Triángulo, Cuadrado, SELECT, START. En las vistas normal y de letras entra SELECT y se cae START. En el visualizador la entrada de estilo ocupa ese lugar y SELECT se cae desde el final, como permite `ui/now-playing`. Se verifica en la consola, en los dos idiomas.

Textos nuevos en `include/lang_strings.h`, en inglés y en español: `STR_HINT_SLEEP_TIMER` ("Sleep" / "Dormir"), `STR_HINT_VIEWS` ("Views" / "Vistas"), `STR_SLEEP_TITLE` ("Sleep timer" / "Temporizador"), `STR_SLEEP_OFF` ("Off" / "Apagado"), `STR_SLEEP_MINUTES` ("%u min", igual en los dos), `STR_SLEEP_END_OF_TRACK` ("End of track" / "Al terminar la pista"), `STR_SLEEP_STATUS_TRACK` ("End of track" / "Fin de pista") y `STR_HINT_CLOSE` ("Close" / "Cerrar"). Cambian `STR_HINT_PREV_NEXT` ("<< >>" en los dos) y `STR_HINT_SHUFFLE` en español ("Mezclar"). `STR_HINT_LYRICS` y `STR_HINT_VISUALIZER` se borran si ninguna otra pantalla los usa.

### D8. Log

- `track` `event=restart` `file=…` en `Music_Previous` cuando reinicia.
- `sleep` `event=set` `mode=…` al elegir en el menú, y `sleep` `event=expire` `mode=…` en `Music_SleepTick` o en `Music_Update` (fin de pista). Al vencer por fin de pista, la línea va después de `track` `event=end` y antes de la `queue` y la `track` `event=open` de la pista que queda en pausa.
- `nowplaying` agrega al final `menu` (`none` o `sleep`) y `sleep` (`SleepTimer_Name`). Se agregan con `LogLine_Str` en `Menu_LogNowPlayingState`, después de `style`, sin renombrar ni quitar ningún campo.

## Recursos y su liberación

- **Temporizador, rampa, master y marcas (`pause_request`, `hold`, start-paused):** son estáticos, viven lo que el proceso, y no se liberan. No se reserva memoria del heap ni se crean hilos ni callbacks.
- **Marca de abrir en pausa:** la pone `Music_Update` y la consume `Audio_Init`. Si ninguna pista abre, `Music_HandleNext` la borra al volver de `TrackAdvance_Run`, de modo que nunca queda viva más allá del avance que la pidió.
- **Master en 0 tras vencer:** lo devuelve a 1 `Music_SleepTick` en cuanto la pausa se cumplió. Si en ese tiempo se descarga la pista (`Audio_Term`) o ninguna pista abre, `Audio_IsSilent()` devuelve verdadero sin pista cargada y el master vuelve a 1 en el fotograma siguiente.
- **Menú del temporizador:** solo estado de la pantalla, sin texturas propias. Se cierra al salir de Reproduciendo.

## Risks / Trade-offs

- **[Carreras entre la UI y el hilo de audio]** → Cada variable compartida la escribe un solo hilo (`pause_request`, `hold` y el master, la UI; `paused` y la rampa, el audio) y son de 32 bits alineados. Una lectura desfasada dura a lo sumo un callback (20 ms), que es el mismo margen que ya acepta `power_tick_thread`.
- **[Los 100 ms se miden en frames de la pista]** → Con el grain de 960 frames, 100 ms son unos 5 callbacks. Una pista mono o de otra frecuencia ajusta los frames con su `rate`, y se prueba en PC con 44,1 y 48 kHz.
- **[Reloj con la app en segundo plano]** → Con el botón PS la consola congela el proceso y la música se corta (visto en el spike de `background-playback`). Si `sceKernelGetProcessTimeWide` sigue avanzando, al volver el temporizador puede haber vencido y pausa enseguida. Si no avanza, sigue contando desde donde quedó. En los dos casos no suena nada mientras tanto, así que se acepta cualquiera y no se especifica.
- **[La leyenda de Reproduciendo ya es larga]** → Pasó: en la consola la entrada del temporizador no se veía en ningún idioma, y acortar su texto no alcanzaba. Se compacta la leyenda como dice D7, y en el visualizador la entrada sigue sin entrar, lo que el spec permite.
- **[Seek a 0 en un decoder]** → Si algún decoder no volviera exactamente al principio, el reinicio empezaría unos ms después. Se verifica con un archivo de cada formato.
- **[Vita3K]** → El emulador se cae con varios badges en pantalla, así que la verificación es en la consola real.

## Estrategia de pruebas

**Compilación:** limpia con `-Wall -Werror` en WSL y en CI.

**Tests en PC (`tests/`, con ASan y UBSan, también en CI):**

- `test_track_advance`: casos nuevos de `TrackAdvance_PrevRestarts` (0, 3000, 3001 y `UINT32_MAX` ms).
- `test_fade` (nuevo): la rampa llega a 0 en exactamente los frames pedidos a 44,1 y 48 kHz, en mono y en estéreo; invertirla a medias sigue desde la ganancia actual sin saltos (la diferencia entre frames consecutivos nunca supera `step` más el redondeo); saturación en ±32767; el atajo con ganancia 1 deja el buffer idéntico; la interpolación del master entre `master_from` y `master_to` es lineal y continua entre dos buffers.
- `test_sleep_timer` (nuevo): con un reloj falso, cada opción vence a su hora una sola vez; ganancia 1 hasta los últimos 10 s y lineal hasta 0; minutos redondeados hacia arriba; reemplazar y cancelar durante el fundido; `SleepTimer_TrackEnded` solo responde en fin de pista y lo apaga; un salto de reloj más allá del vencimiento vence una vez; los nombres del log.
- `test_lang`: las cadenas nuevas existen en los dos idiomas (lo cubre el test de siempre).

**Archivos de prueba para la consola:** con ffmpeg en Windows, en `ux0:/pruebas-eleven/niceties/`: `01 tono 8s.flac`, `02 tono 8s.flac` y `03 tono 8s.flac` (tres pistas cortas para el fin de pista), y `04 tono 60s.flac` (para esperar 5 s sin que termine). Para el formato de cada decoder se usa `ux0:/pruebas-eleven/formatos/`, y para escuchar el fundido, canciones de `ux0:/Mis cosas/`. Al terminar se borra `niceties/` de la tarjeta.

**Suite en la consola (`tests/console/playback-niceties.txt`):**

| Escenario | Cómo lo verifica el log |
|---|---|
| Retroceder con más de 3 s reinicia (L) | `sleep 5s`, `press L`, `wait track event=restart file=…` |
| Retroceder con menos de 1 s pasa a la anterior | `press L` enseguida, `wait queue pos=…` y `wait track event=open file=…` de la anterior |
| Dos retrocesos seguidos | `wait track event=restart`, y luego `wait track event=open` de la anterior |
| Con la repetición encendida | `press SQUARE`, `sleep 5s`, `press L`, `wait track event=restart` |
| Reiniciar en pausa no reanuda | `press CROSS`, `wait nowplaying paused=1`, `press L`, `wait track event=restart`, `wait nowplaying paused=1` |
| El botón de anterior en pantalla y el del mini reproductor | `tap` en sus coordenadas, `wait track event=restart` |
| Abrir, elegir y cancelar el temporizador | `press SELECT`, `wait nowplaying menu=sleep sleep=off`, `press DOWN`, `press CROSS`, `wait sleep event=set mode=15`, `wait nowplaying menu=none sleep=15`, y lo mismo hacia "Apagado" |
| Los gatillos no actúan con el menú abierto | `press SELECT`, `press R`, `press CIRCLE`, `wait nowplaying menu=none file=<la misma pista>`: si R hubiera cambiado de pista, la última `nowplaying` tendría otro `file` y el `wait` fallaría |
| Fin de pista | elegir `track`, `wait track event=end timeout=15s`, `wait sleep event=expire mode=track`, `wait nowplaying file=…02… paused=1 sleep=off` |
| Fin de pista fuera de Reproduciendo | lo mismo, volviendo a Carpetas antes de que termine, y `wait sleep event=expire mode=track` |
| Vencer 15 minutos | `04 tono 60s.flac` con la repetición encendida, elegir 15, `wait sleep event=expire mode=15 timeout=960s`, `wait nowplaying paused=1 sleep=off` |

**Verificación manual en la consola** (lo que el log no ve o el guion no puede hacer):

- El fundido de pausa y de reanudación, sin clic, con auriculares, y pausar y reanudar enseguida (audio).
- El fundido de los últimos 10 s y que siga en la pista nueva si cambia a mitad (audio).
- Cancelar durante el fundido final (audio).
- Buscar con la barra, sonando y en pausa, sin fundidos ni cambios de estado (audio, más `nowplaying paused` sin cambios en el log).
- El indicador en la barra de estado en los dos idiomas, con el redondeo de minutos y sin solaparse (imagen).
- La leyenda en los dos idiomas, y la leyenda con el menú abierto (imagen).
- El menú sobre las vistas de letras y de visualizador (imagen).
- El modo de fin de pista con la pantalla apagada y el encendido con PS (el guion no puede apagar ni encender la pantalla de verdad).
- Reabrir la app desde su LiveArea con un temporizador activo (cierra el proceso, que el guion no puede hacer).
- Reiniciar una pista de cada formato de `formatos/` (audio: que empiece exactamente al principio).

**Líneas de `debug.log` que verifican cada escenario:**

| Escenario | Líneas |
|---|---|
| `playback/queue`: retroceder a mitad reinicia, con repetición, el mini reproductor, el botón en pantalla | `track` `event=restart` con el `file` de la pista, sin `queue` ni `track` `event=open` antes |
| `playback/queue`: retroceder al principio pasa a la anterior, extremos circulares, barajado | `queue` con el `pos` anterior, `track` `event=open` con su `file` |
| `playback/queue`: reiniciar en pausa | `track` `event=restart` y `nowplaying` con `paused=1` antes y después |
| `playback/sleep-timer`: elegir, reemplazar, cancelar | `sleep` `event=set` con su `mode`, y `nowplaying` con su `sleep` |
| `playback/sleep-timer`: vencer por tiempo, en pausa, en otra pantalla | `sleep` `event=expire` con el `mode`; `nowplaying` con `paused=1 sleep=off` |
| `playback/sleep-timer`: fin de pista, con repetición, reanudar | `track` `event=end`, `sleep` `event=expire mode=track`, `track` `event=open` de la pista que queda, `nowplaying` con `paused=1`; al reanudar, `nowplaying` con `paused=0` y luego `track` `event=end` y `event=open` de la tercera |
| `playback/sleep-timer`: indicador, no se recuerda | visual; tras reabrir, la primera `nowplaying` del proceso tiene `sleep=off` |
| `playback/pause-fade` | visual y de oído; el log solo confirma `nowplaying` `paused` en el fotograma de la pulsación |
| `ui/now-playing`: menú y leyenda | `input` con `down=SELECT`, `nowplaying` con `menu=sleep`/`menu=none` |

## Pipeline de CI

No hay cambios en el workflow. `test_fade` y `test_sleep_timer` se agregan a `BIN` y a sus reglas en `tests/Makefile`, que CI ya ejecuta, y `fade.c` y `sleep_timer.c` entran en el `CMakeLists.txt`.

## Licencias

No se incorpora código ni assets de terceros. Todo es código propio bajo GPL-3.0-or-later.

## Migration Plan

No aplica. No hay datos en disco nuevos ni cambios de formato. Para volver atrás basta con instalar el `.vpk` anterior.
