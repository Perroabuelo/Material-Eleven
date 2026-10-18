## Context

Ver `proposal.md` para la motivación y `specs/ui/visualizer/spec.md` para el comportamiento.

Estado actual que condiciona el diseño:

- **Camino del audio.** Hay un solo canal de salida (`VITA_NUM_AUDIO_CHANNELS 1`). Su hilo (`vitaAudioChannelThread`, prioridad 0x40) llama al callback `Audio_Decode` para llenar un buffer de `grain` frames y lo entrega con `sceAudioOutOutput`, que bloquea. Hay doble buffer, y el grain por defecto es 960 frames (20 ms a 48 kHz, 21,8 ms a 44,1 kHz). El hilo se recrea en cada pista (`vitaAudioInit` sube `audio_generation`). El EQ y el ALC los aplica la consola después de `sceAudioOutOutput`.
- **Mono.** El buffer mide `grain * canales` shorts. Ya hubo un desborde del heap por suponer estéreo (comentario en `Audio_Decode`), así que el ancho de un frame se toma siempre de `vitaAudioGetChannelCount()`.
- **Reproduciendo** (`menu_audioplayer.c`) tiene dos vistas, decididas por `static SceBool lyrics_view`. La vista de letras ya resuelve la cabecera compacta, la barra de progreso propia (`lyrics_seek_state`) y la memoria de sesión. El loop no usa ↓, ← ni →.
- **Dibujo.** Toda geometría pasa por `UI_GpuPoolAlloc` (pool por fotograma; si se agota, se cuenta en el overlay). `UI_DrawRing` usa 24 segmentos fijos, pocos para un disco de unos 200 px. vita2d de VitaSDK trae `vita2d_draw_array_textured`, con vértices que llevan UV.
- **Gestos.** `gesture.c` reconoce arrastre vertical (`dy`), toque y mantener. No reconoce un deslizamiento horizontal.
- **Pantalla apagada.** `Utils_ReadControls` llama a `scePowerRequestDisplayOff()`. La pantalla se vuelve a encender con el botón PS, fuera de la app, y el loop de fotogramas sigue corriendo.

## Goals / Non-Goals

**Goals:**

- Que toda la matemática (buffer circular, FFT, bandas, medidores, disparo del osciloscopio, giro, geometría del disco) sea código puro sin vita2d ni SCE, probado en PC con ASan y UBSan.
- Que el hilo de audio haga en la toma solo una copia acotada, sin locks, sin reservar memoria y sin llamadas al sistema.
- Que el visualizador no reserve memoria del heap: todo su estado es estático y de tamaño fijo.
- Que el costo por fotograma del estilo más pesado entre en el presupuesto de 60 fps junto con el resto de Reproduciendo.

**Non-Goals:**

- Compensar el EQ y el ALC de la consola. El visualizador muestra la señal antes de ellos.
- Calibrar la latencia en cada consola o con cada salida de audio (altavoz, auriculares, Bluetooth). Se fija una sola latencia por diseño (ver D2).
- Optimizar la FFT con NEON a mano. El compilador con `-O3` alcanza para el tamaño elegido.

## Decisions

### D1. La toma va en `Audio_Decode`, hacia un buffer circular de un escritor y un lector

`Audio_Decode` (audio.c), después de decodificar o de escribir silencio, copia el buffer al buffer circular del visualizador (`viz_tap`), convertido a estéreo: un frame mono se escribe en los dos canales. El buffer circular es estático, de 8192 frames estéreo `int16` (32 KB). El escritor es solo el hilo de audio. Primero copia las muestras y después publica el contador total de frames escritos (`uint32_t`, con barrera `__sync_synchronize()` entre los dos pasos). El lector es solo la UI: lee el contador y copia una ventana que termina en la posición que se está oyendo (D2). Como lee hasta 2048 frames detrás de un escritor que va unos pocos grains adelante, sobra margen antes de que el escritor dé la vuelta. Si aun así el contador avanzó más de `capacidad - ventana` durante la copia, el lector descarta ese fotograma y repite el anterior.

Junto al contador, el escritor publica la frecuencia de muestreo de la pista. La UI reinicia el buffer circular en `Audio_Init`, antes de `vitaAudioInit`, cuando todavía no corre ningún hilo de la pista nueva. Los hilos viejos que quedan pendientes ya no llaman al callback, porque `vitaAudioThreadIsCurrent` lo impide.

*Alternativas:* tomar las muestras en `vitaAudioChannelThread`, justo antes de `sceAudioOutOutput`. Es el mismo punto en la práctica, pero mezcla la lógica del visualizador con la librería de audio heredada. Un mutex queda descartado porque podría bloquear el hilo de audio. Reservar el buffer con `malloc` en cada pista agregaría un recurso que liberar sin ningún beneficio.

### D2. Sincronía: el lector lee con un retraso fijo de "lo que hay en cola"

Lo que se oye va detrás de lo decodificado. Se acumulan el buffer que se está llenando, el que entregó `sceAudioOutOutput` y la cola interna de la consola. La UI calcula la posición oída como `escritos − (2 × grain + VIZ_HW_LATENCY_FRAMES)` y toma la ventana de análisis terminada en ese punto. `VITA_DEFAULT_AUDIO_SAMPLES` y el grain configurado vienen de `vitaAudioGetGrain()`. `VIZ_HW_LATENCY_FRAMES` es una constante con un valor inicial de un grain, que se ajusta en la consola con el archivo de pulsos (ver Estrategia de pruebas) hasta que el desfase no se note.

*Alternativa:* `sceAudioOutGetRestSample(port)` da los frames que le quedan al puerto, y la posición sería más exacta. Exige exponer el puerto del hilo vigente a la UI y es una llamada al sistema por fotograma. Se deja como mejora si el retraso fijo no basta en la prueba de pulsos; no cambia los specs.

### D3. Análisis puro: FFT radix-2 propia de 1024 puntos

- **`viz_fft.c`:** FFT compleja iterativa radix-2 en `float`, de 1024 puntos, con tablas de seno y de reversión de bits calculadas una sola vez en arrays estáticos. Ventana de Hann. La entrada es el promedio L+R de la ventana.
- **`viz_bands.c`:** 32 bandas con bordes logarítmicos entre 40 Hz y 16 kHz, calculados según la frecuencia de muestreo publicada. Una banda más angosta que un bin toma el bin que la contiene, de modo que los graves no quedan vacíos. El nivel de cada banda está en dB, normalizado al rango −60…0 dBFS.
- **`viz_meter.c`:** suavizado de ataque inmediato y caída exponencial dependiente de `dt`, para que la velocidad no dependa de los fps. Marca de pico con retención de 0,8 s y caída posterior. Lo usan las barras y los VU.
- **VU:** RMS por canal sobre los últimos 50 ms de la ventana, en dB, con escala −48…0.
- **Energía de graves:** suma de las bandas por debajo de 150 Hz, con una ganancia automática lenta (máximo con caída de unos segundos), para que el latido funcione igual en un máster fuerte y en uno suave.
- **Osciloscopio:** en una ventana de 2048 frames se busca el primer cruce por cero ascendente dentro de la primera mitad, y desde ahí se dibujan 512 frames. Así una onda periódica queda quieta. Si no hay cruce, se dibuja desde el principio de la ventana.

Una FFT por fotograma de 1024 puntos son unas 25 000 operaciones en float, algo despreciable para el Cortex-A9.

*Alternativa:* kissfft (BSD-3, compatible con GPLv3). Funcionaría, pero obliga a registrar en NOTICE y LICENSES/ un código que tiene el tamaño de una función. Se escribe una propia, y el test en PC la compara con una DFT directa.

### D4. El vinilo: abanico texturizado con UV giradas

- **Etiqueta:** abanico de `VIZ_DISC_SEGMENTS = 96` triángulos con `vita2d_draw_array_textured` sobre `metadata.cover_image`. Los vértices del borde son fijos en pantalla. Lo que gira son las UV: `uv = 0.5 + 0.5·(cos(θ+φ), sin(θ+φ))`. La carátula gira "dentro" del círculo sin recalcular el polígono. La geometría (`viz_disc.c`: posiciones y UV para un radio, centro, segmentos y ángulo dados) es pura y se prueba en PC.
- **Disco, surcos y agujero:** geometría vectorial. El disco es un círculo relleno casi negro. Los surcos son 6 anillos finos de baja opacidad. La etiqueta sin carátula es un círculo del color de acento, y el agujero es un círculo del color de fondo.
- **Suavizado del borde:** se dibuja un anillo de 1,5 px sobre el borde de la etiqueta y otro sobre el del disco, con alfa decreciente hacia afuera. Así se cumple el suavizado de `ui/rendering` sin depender del MSAA.
- **Segmentos:** `UI_DrawRing` gana una variante con cantidad de segmentos (`UI_DrawRingN`), porque 24 se notan como polígono a ese tamaño. La firma actual de `UI_DrawRing` se mantiene.
- **Giro (`viz_spin.c`, puro):** velocidad angular objetivo de 33⅓ rpm sonando y 0 en pausa. La velocidad se acerca al objetivo con una constante de tiempo de 0,5 s, de modo que frena en un par de segundos. El ángulo se integra con `dt`, en módulo 2π para no perder precisión con el tiempo.

*Alternativas:* rotar los vértices en vez de las UV, que es lo mismo pero con más cuentas. `vita2d_draw_texture_scale_rotate` gira un cuadrado y no recorta en círculo. Un render a textura con máscara exigiría una textura intermedia que crear y liberar.

### D5. Estilos radial y respiración

- **Radial:** usa las mismas 32 bandas espejadas a 64 barras alrededor del borde (graves arriba, agudos abajo por ambos lados), para que la figura quede simétrica. Cada barra es un cuadrilátero, 4 vértices en `TRIANGLE_FAN`, y las barras se agrupan en una sola llamada con `TRIANGLES` (6 vértices por barra) para no hacer 64 llamadas.
- **Respiración:** escala del disco `1 + 0,06·e`, donde `e` es la energía de graves normalizada y suavizada. El halo son 3 anillos concéntricos del color de acento, con alfa y radio proporcionales al nivel RMS.

### D6. Estado de la vista y del estilo

`lyrics_view` se reemplaza por `static NowPlaying_View np_view` (`NP_VIEW_NORMAL`, `NP_VIEW_LYRICS`, `NP_VIEW_VISUALIZER`) y `static Viz_Style viz_style` (inicial `VIZ_STYLE_SPECTRUM`). Las transiciones (↑, ↓, ← y →, con la vuelta) son una función pura en `viz_nav.c` con test en PC. Lo que hacía `if (lyrics_view)` pasa a ser un `switch`. Ninguno de los dos se guarda en `config.cfg`.

### D7. Deslizamiento horizontal

`Gesture_Update` agrega el resultado `GESTURE_SWIPE_X`, con dirección, cuando un toque que empezó dentro de la zona del visualizador recorre más de 80 px en horizontal antes de 400 ms y con más desplazamiento horizontal que vertical. Se prueba en PC junto con los casos que ya cubre `test_gesture`. La zona excluye la barra de progreso y el nav rail, que se atienden antes, igual que en la vista de letras.

### D8. Pantalla apagada

El análisis y el dibujo del visualizador corren solo dentro del dibujo de la vista. Para no hacer trabajo con la pantalla apagada, `utils.c` expone si la app pidió apagar la pantalla, y el visualizador no analiza mientras eso esté activo. Para saber cuándo se volvió a encender, se registra un callback de energía (`scePowerRegisterCallback`) que borra la marca al encender la pantalla. Al volver, la ventana de análisis toma lo que suena en ese momento (D2), así que no hay retraso acumulado. Si el callback no informa el encendido de la pantalla en este firmware, se usa el respaldo: `sceDisplayWaitVblankStart` deja de avanzar con la pantalla apagada, y el loop puede detectar el salto de tiempo entre fotogramas. Ver Open Questions.

### D9. Leyenda

`NavRail_Hint` gana `HINT_BTN_DOWN` (flecha abajo) y `HINT_BTN_LEFT_RIGHT` (un chip con dos flechas), dibujados como geometría vectorial con el mismo alto que el chip de ↑. El orden en Reproduciendo es: confirmar, volver, L/R, ↑ Letras, ↓ Visualizador, ←→ Estilo (solo en el visualizador), △, □, START. Como la barra corta desde el final, lo primero que se pierde es START.

## Recursos y su liberación

- **Buffer circular, tablas de la FFT, buffers de análisis y estado de los estilos:** son estáticos, viven lo que dura el proceso y no se liberan. No se reserva memoria del heap ni se crean hilos.
- **Callback de energía (D8):** se crea una vez al arrancar en el hilo de UI (`sceKernelCreateCallback` y `scePowerRegisterCallback`), sin hilo propio. El loop de fotogramas lo atiende con `sceKernelCheckCallback()` en `Utils_ReadControls`, y se libera al cerrar con `scePowerUnregisterCallback` y `sceKernelDeleteCallback`. Si el registro falla, se deja la marca de pantalla apagada sin efecto: el visualizador analiza siempre, que es más costoso pero correcto.
- **Carátula:** el visualizador no tiene textura propia. Dibuja `metadata.cover_image`, que la pista crea y libera como hoy, y el loop la deja de referenciar en el mismo punto en que lo hace la vista normal. Los vértices salen del pool del fotograma (`UI_GpuPoolAlloc`). Si el pool se agota, el estilo dibuja lo que entró, y el overlay suma el agotamiento como ya hace con el resto.

## Risks / Trade-offs

- **[El retraso fijo no coincide en todas las salidas]** → Con auriculares Bluetooth la latencia es mayor y el visualizador se verá adelantado. Se acepta: es una limitación conocida que se anota en el README.
- **[El visualizador no ve el EQ]** → Con un preset que realza graves, las barras no lo reflejan. Queda fuera de alcance y se anota en el README.
- **[El pool de vértices de un fotograma]** → El estilo radial con disco, surcos, halo y barras es el más pesado, con unos 2 500 vértices. Mitigación: medirlo con el overlay (uso del pool) en la consola, y si se acerca al límite, bajar segmentos de surcos antes que de la etiqueta.
- **[Vita3K]** → El emulador se cae con varios badges en pantalla (problema conocido), así que la verificación es en la consola real.
- **[Hilo de audio y caché]** → La copia al buffer circular son unos 3,8 KB por grain, nada frente a decodificar FLAC. El riesgo de un corte en el audio se verifica en la prueba de una canción entera con el estilo radial.
- **[Disco sin MSAA]** → Si los anillos de suavizado no alcanzan, el borde de la etiqueta texturizada puede parpadear al girar. Mitigación: hacer un poco transparente el último vértice del abanico, porque el color del vértice modula la textura.

## Estrategia de pruebas

**Compilación:** limpia con `-Wall -Werror` en WSL y en CI.

**Tests en PC (`tests/`, con ASan y UBSan, también en CI):**

- `test_viz_tap`: escritura mono y estéreo, vuelta del buffer, lectura con retraso, descarte si el escritor dio la vuelta, reinicio.
- `test_viz_fft`: tonos sintéticos de 100 Hz, 1 kHz y 8 kHz a 44,1 y 48 kHz dan su máximo en el bin esperado, y la FFT coincide con una DFT directa.
- `test_viz_bands`: los bordes son logarítmicos y crecientes, ninguna banda queda vacía, y un tono enciende la banda correcta. El silencio da −60 dB.
- `test_viz_meter`: ataque inmediato, caída que no depende de `dt` (60 frente a 30 fps dan el mismo valor al cabo de 1 s), y retención y caída del pico. El RMS de un canal en silencio da el piso.
- `test_viz_scope`: el disparo en un cruce ascendente deja una senoide en la misma fase en ventanas sucesivas.
- `test_viz_spin`: frena hasta 0 en menos de 2,5 s, acelera hasta el objetivo, y el ángulo queda acotado tras horas simuladas.
- `test_viz_disc`: UV dentro de [0,1], borde a radio exacto, y el giro desplaza las UV sin mover las posiciones.
- `test_viz_nav`: todas las transiciones de vista y de estilo de los specs, con la vuelta.
- `test_gesture`: los casos nuevos de deslizamiento horizontal, y que el arrastre vertical no se dispare como deslizamiento.

**Archivos de prueba para la consola:** se generan con ffmpeg en Windows en `ux0:/pruebas-eleven/visualizador/`:

- `tono-100hz.flac`, `tono-8khz.flac` y `tono-440hz.flac`, mono.
- `solo-izquierdo.wav`, estéreo, con tono en L y silencio en R.
- `pulsos.wav`, un pitido de 50 ms cada segundo.

La música con batería sale de `ux0:/Mis cosas/`. Al terminar las pruebas se borra `visualizador/` de la tarjeta.

**Suite en la consola (`suite.txt`):** cubre la navegación con `press` y `wait nowplaying view=… style=…`: abrir y cerrar, el salto entre letras y visualizador, los cinco estilos hacia adelante y la vuelta hacia atrás, la memoria al ir a Ajustes y volver, y la memoria tras `R`. El escenario de memoria repite el recorrido y compara `heap_kb` de las líneas `screen`.

**Verificación manual en la consola:** los aspectos visuales y de sonido que el log no puede ver:

- La forma de cada estilo con los tonos.
- La sincronía con `pulsos.wav`, para ajustar `VIZ_HW_LATENCY_FRAMES`.
- El giro y el frenado del vinilo.
- El borde suavizado.
- El uso del pool en el overlay con el estilo radial.
- Una canción entera sin cortes.
- Pantalla apagada y encendida.

**Líneas de `debug.log` que verifican cada escenario:**

| Escenario de `ui/visualizer` | Líneas |
|---|---|
| Abrir y cerrar, salto directo, recorrer estilos, flechas en la vista normal | `input` con `down=DOWN`/`UP`/`LEFT`/`RIGHT`, seguidas de `nowplaying` con el `view` y el `style` esperados |
| Cambiar de estilo deslizando | `touch` `event=down`/`up` y `nowplaying` con el `style` nuevo, sin una línea `queue` ni `track` entre medio |
| Memoria en la sesión | `screen` `to=settings`, `screen` `to=nowplaying` y luego `nowplaying` con el mismo `view` y `style` |
| Estilo al cambiar de pista | `queue` y `track` `event=open`, y luego `nowplaying` con el `file` nuevo y el mismo `style` |
| Al arrancar no se recuerda | primera línea `nowplaying` del proceso nuevo con `view=normal style=spectrum` |
| Botones en el visualizador | `nowplaying` `paused=1` y luego `paused=0`, con `view=visualizer` |
| Todos los formatos y mono | `track` `event=open` `result=ok` con su `format` y `channels`, y `track` `event=end` de cada uno, sin una línea `log` `event=start` intermedia (que indicaría un cierre) |
| No acumula memoria | `heap_kb` de las líneas `screen` `to=folders` |
| Estilos, sincronía, vinilo, pantalla apagada | verificación visual; el log solo confirma `view` y `style` |

Solo se agregan los campos `view` y `style` al final de `nowplaying`, como define el delta de `dev/debug-log`. No se renombra ni se quita ningún campo.

## Pipeline de CI

No hay cambios en el workflow. Los tests nuevos se agregan a `tests/Makefile`, que CI ya ejecuta, y los fuentes nuevos de `source/` entran en el `CMakeLists.txt`.

## Licencias

No se incorpora código ni assets de terceros. La FFT y la geometría del disco son código propio bajo GPL-3.0-or-later.

## Migration Plan

No aplica. No hay datos en disco nuevos ni cambios de formato. Para volver atrás basta con instalar el `.vpk` anterior.

## Open Questions

- ¿El callback de energía informa el encendido de la pantalla en este firmware? Si no, se usa el respaldo del salto de tiempo entre fotogramas (D8). Ninguna de las dos opciones cambia los specs ni las tareas.
  - **Resuelta (tarea 1.3):** sí. Al encender la pantalla con PS llega `SCE_POWER_CB_APP_RESUMING`. El respaldo no habría servido: con la pantalla apagada el loop de fotogramas sigue a ritmo normal, sin saltos de tiempo.
- ¿Cuál es el valor final de `VIZ_HW_LATENCY_FRAMES`? Se ajusta en la consola con `pulsos.wav`.
  - **Resuelta (tarea 4.6):** 960 frames (un grain). Coincide con el altavoz y con audífonos de cable.
