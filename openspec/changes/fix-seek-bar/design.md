## Context

Ver proposal.md, "Why", para el problema. Estado actual relevante:

- **Geometría.** Barra principal: x 500..926 (426 px), `SEEK_Y` 220, 5 px de alto, zona táctil `SEEK_Y ± 12`. Barra de letras: `LYRICS_X`..`LYRICS_X + LYRICS_W`, `LYRICS_SEEK_Y` 474, misma altura y misma zona. El arrastre de letras empieza solo si el toque cae por encima de `LYRICS_SEEK_Y - 12`.
- **Seek.** `Audio_Seek(SceUInt64 index)` delega en `*_Seek`, que hace `total * (index / 450.0)` y fija su contador de posición (`frames_read` o `samples_read`). XM convierte a milisegundos con `/ 44.1`.
- **Touch.** `touch.c` da `pressed` (flanco: solo el primer fotograma), `held` y `released` (flanco). `Touch_Position` y `UI_TouchTarget` exigen `pressed`. Por eso la barra solo atiende el primer fotograma, y por eso los botones de transporte no se disparan a mitad de un arrastre.
- **Vecinos.** Sobre la barra principal, en el panel derecho, no hay nada táctil. Debajo, la zona del botón de play empieza en y 280, y las de los botones laterales, ya crecidas a `UI_TOUCH_MIN`, en y 285. Bajo la barra de letras, la barra de hints empieza en y 504 y no es táctil.

## Goals / Non-Goals

**Goals:**
- Una sola fuente de verdad para "X en pantalla → fracción de la pista", con el ancho real de cada barra.
- La lógica de agarrar, seguir y soltar en un módulo puro, probado en PC, compartido por las dos barras.
- Que el dibujo responda en el mismo fotograma en que se mueve el dedo.

**Non-Goals:**
- Cambiar `touch.c`. Sus flancos ya bastan.
- Cambiar cómo se reporta la posición durante la reproducción normal.
- Animar la transición entre grosor de reposo y grosor agarrado. Pasa de golpe: es lo que se siente "snappy" y no agrega estado.

## Decisions

### 1. `Audio_Seek` recibe una fracción `double` en [0, 1]

`SceUInt64 Audio_Seek(double fraction)`. `audio.c` acota a [0, 1] y cada `*_Seek(double fraction)` calcula `target = (uint64)(total * fraction)`, acotado a `total - 1` cuando `total > 0`. El 450 desaparece de los seis decodificadores. XM conserva su conversión a milisegundos.

Acotar a `total - 1` hace que "tocar el final" deje un frame por leer: el siguiente callback lo decodifica, marca el fin de pista y la cola avanza (o repite) por el camino de siempre. Así se evita que un seek justo a `total` falle en algún decodificador y no haga nada.

- *Alternativa: posición en samples o en ms.* Obliga a la UI a conocer el largo en la unidad de cada decodificador. La fracción es lo que la barra produce de forma natural.
- *Alternativa: dejar el 450 y escalar en cada vista,* como hace hoy la vista de letras. Es lo que causó el bug: el número mágico vive lejos de quien lo usa.
- *`double` y no `float`:* una pista de una hora a 48 kHz tiene unos 173 M de samples. Con `float` el error ronda las decenas de samples. Es inaudible, pero con `double` no hay que pensarlo.

### 2. Módulo puro `seek_bar`: estado, entrada y comandos

```
include/seek_bar.h / source/seek_bar.c   (sin vita2d ni SCE)

typedef struct {
    float x, w;          // extremo izquierdo y ancho dibujado
    float hit_top, hit_bottom;   // zona táctil vertical
} SeekBar_Geometry;

typedef struct {
    int   grabbed;
    float fraction;      // posición del dedo mientras está agarrada
} SeekBar_State;

typedef struct { int pressed, held, released; float x, y; } SeekBar_Input;

typedef enum { SEEKBAR_IDLE, SEEKBAR_DRAGGING, SEEKBAR_COMMIT } SeekBar_Result;

SeekBar_Result SeekBar_Update(SeekBar_State *, const SeekBar_Geometry *,
                              const SeekBar_Input *, double *out_fraction);
float SeekBar_FractionAt(const SeekBar_Geometry *, float x);   // acotada a [0,1]
float SeekBar_DisplayFraction(const SeekBar_State *, float playback_fraction);
```

```
          pressed dentro de la zona
 [IDLE] ----------------------------> [GRABBED]  fraction = FractionAt(x)
   ^                                     |  held: fraction = FractionAt(x)
   |                                     |        (y se ignora)
   +------------- released --------------+  -> COMMIT(fraction)
```

- Agarra solo un `pressed` dentro de `[x, x + w] × [hit_top, hit_bottom]`. Un `pressed` fuera no hace nada.
- Mientras está agarrada, cada fotograma con `held` actualiza la fracción. La `y` no importa.
- `released` (o un fotograma sin `held`) suelta y devuelve `SEEKBAR_COMMIT` con la fracción final. No hay seek mientras se arrastra.
- `w <= 0` nunca agarra: no hay división por cero.
- `SeekBar_DisplayFraction` devuelve la del dedo si está agarrada y si no la de la reproducción. El dibujo y el texto del tiempo usan esa.

Los menús arman `SeekBar_Input` desde `Touch_*` y aplican el resultado: en `COMMIT` hacen la danza actual de pausa, `Audio_Seek(fraction)` y reanudación. El módulo no sabe nada de audio.

- *Alternativa: escribirlo dentro de `menu_audioplayer.c`.* Duplicaría la lógica entre las dos barras y no se podría probar en PC, contra la regla del proyecto.

### 3. Zona táctil de `UI_TOUCH_MIN` (66 px) centrada en la línea

- **Principal:** `hit_top = SEEK_Y + SEEK_H/2 - 33` (≈ 189) y `hit_bottom` ≈ 255. No se cruza con la zona del play (≥ 280). Las horizontales no crecen: la barra ya es ancha, y crecer a los lados invadiría el panel izquierdo.
- **Letras:** misma fórmula sobre `LYRICS_SEEK_Y`, que da ≈ 443..509. Se acota `hit_bottom` al borde de la barra de hints (504): queda en 61 px, aceptado porque esa barra no es táctil y debajo no hay nada. El límite del arrastre de letras (`Touch_GetY() < LYRICS_SEEK_Y - 12`) pasa a ser `< hit_top`, para que las dos zonas no se pisen.
- **Orden de prueba:** en Reproduciendo, `Menu_HandleTransportTouch` se sigue probando antes. Un `pressed` que toma un botón no llega a la barra. Una vez agarrada, los botones ya no ven `pressed`, así que no se disparan aunque el dedo pase sobre ellos.

### 4. Dibujo

- **Reposo:** igual que hoy, pista `SEEK_H` = 5 px y relleno de acento.
- **Agarrada:** la pista y el relleno se dibujan con 8 px de alto, centrados en la misma línea para que el texto de abajo no se mueva. Encima va un thumb circular de radio 8 px en color de acento, centrado en `x + w * fraction`. El círculo se dibuja con `vita2d_draw_fill_circle`, que ya está disponible en vita2d.
- **Relleno estrecho:** cerca del inicio, el ancho del relleno puede ser menor que su alto. `UI_DrawRoundedRect` ya acota el radio, así que se mantiene. Si en la consola se ve un "grano" raro con el relleno casi vacío, se resuelve con un ancho mínimo de relleno igual al alto, sin cambiar el comportamiento.
- **Tiempo transcurrido:** mientras está agarrada, se muestra `fraction × Audio_GetLengthSeconds()`. La duración total no cambia.
- **Vista de letras:** al hacer `COMMIT` se llama `Menu_LyricsSnap()` (que también pone `lyrics_moved` en falso), así las letras saltan a la línea nueva en vez de deslizarse desde la vieja. Mientras se arrastra, las letras siguen la posición real, que no cambia.

### 5. Recursos

El cambio no reserva memoria, no abre archivos ni crea hilos. El estado de cada barra es un `SeekBar_State` estático en `menu_audioplayer.c`. Se reinicia (`grabbed = 0`) al entrar a Reproduciendo, al alternar la vista de letras y al cambiar de pista, para que un arrastre no sobreviva a esos cambios y mande un seek a otra pista.

## Risks / Trade-offs

- **[Seek concurrente con el callback de audio]** `Audio_Seek` corre en el hilo de la UI mientras el hilo de audio puede estar dentro de `*_Decode`. Invertir `paused` no espera a que termine un decode en curso. → Sigue habiendo un seek por gesto, igual que hoy, así que el riesgo no crece. Queda fuera de alcance (proposal.md) y anotado aquí para un cambio futuro: un lock o una solicitud de seek que el propio callback atienda.
- **[El seek pasa del toque al soltar]** Un toque simple ahora salta al levantar el dedo, no al apoyarlo: unos 100 ms después. → Es imperceptible, y es lo que permite arrastrar sin mandar seeks intermedios.
- **[MP3 sin índice: seek lento]** `mpg123_seek` puede tardar en archivos grandes sin tabla. → Un solo seek por gesto, igual que hoy.
- **[Cambio de firma en siete archivos]** `Audio_Seek` y los seis `*_Seek`. → Lo detecta el compilador con `-Werror`. Se hace en una sola tarea.
- **[Zona de letras de 61 px]** Un poco menor que `UI_TOUCH_MIN`. → Aceptado: se acota contra la barra de hints, que no es táctil.

## Estrategia de pruebas

- **Compilación:** `.vpk` limpio con `-Wall -Werror` (local en WSL y en CI).
- **Tests en PC (`tests/test_seek_bar.c`, con ASan y UBSan):**
  - `FractionAt`: borde izquierdo 0, borde derecho 1, centro 0,5, fuera a la izquierda o a la derecha acotado, `w <= 0` sin división por cero.
  - Toque sin arrastre: `pressed` dentro y luego `released` da `COMMIT` con la fracción del punto.
  - Arrastre: `pressed` en 0,25, varios `held` hasta 0,9, `released` da un solo `COMMIT(0,9)`. Los fotogramas intermedios devuelven `DRAGGING` y nunca `COMMIT`.
  - El dedo sale en vertical (`y` fuera de la zona) mientras sigue `held`: sigue arrastrando.
  - `pressed` fuera de la zona (arriba, abajo, a los lados) no agarra.
  - `DisplayFraction`: la del dedo si está agarrada, la de la reproducción si no.
  - Reinicio del estado: después de reiniciar, un `released` suelto no da `COMMIT`.
- **Consola (ver los escenarios de la spec, paso a paso):** tocar el centro, el borde derecho y el izquierdo; arrastrar y mantener; salir de la barra en vertical; la zona más alta; la barra de letras con letras sincronizadas. Un toque en cada formato (FLAC, MP3, OGG, Opus, WAV, XM) sobre el borde derecho, para confirmar que todos llegan al final. No se usa Vita3K: según la experiencia previa se cae con esta pantalla.
- Como no se reserva memoria, no hace falta la medición de HEAP.

## CI

No hace falta cambiar el pipeline. El job de tests ya corre `make -C tests`, así que basta con sumar `test_seek_bar` a `BIN` en `tests/Makefile`.

## Migration Plan

No hay datos persistentes. Para volver atrás basta con revertir el PR.
