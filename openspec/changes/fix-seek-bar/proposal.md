## Why

La barra de progreso de Reproduciendo se dibuja con un ancho y se toca con otro. Mide 426 px (`RIGHT_PANEL_R - RIGHT_PANEL_X`), pero el toque llega a `Audio_Seek` como el píxel sin escalar, y cada decodificador lo interpreta sobre una barra fija de 450 px (`index / 450.0` en flac, mp3, ogg, opus, wav y xm). Al tocar el borde derecho se queda en el 94,7 % de la pista y nunca se llega al final. Además, el relleno queda atrás del dedo, hasta unos 24 px en el extremo. El 450 es un resto del layout heredado. La vista de letras lo compensa a mano y la vista principal no.

La barra además se siente rígida. `Touch_Position` exige `Touch_CheckPressed()`, que solo vale el primer fotograma del toque, así que no se puede arrastrar: solo responde un toque puntual. La zona táctil mide 24 px de alto, muy por debajo de `UI_TOUCH_MIN` (66 px). Tampoco hay respuesta visual al tocar: no aparece un indicador y la barra no cambia.

## What Changes

- **El seek se expresa como fracción de la pista,** no como un píxel sobre una barra de 450 px. `Audio_Seek` recibe una posición entre 0 y 1, y cada decodificador la convierte a su propia unidad. Desaparece el 450 de los seis decodificadores. Cada vista calcula la fracción con el ancho real de su barra, así que tocar el borde derecho lleva al final de la pista.
- **La barra se puede arrastrar.** Un toque sobre la barra la "agarra". Mientras el dedo sigue apoyado, el relleno y el tiempo transcurrido siguen al dedo, aunque el dedo se salga de la barra en vertical (solo cuenta la X). El seek se hace una sola vez, al levantar el dedo. Un toque sin arrastre salta a ese punto, como hoy.
- **Respuesta visual al tocar.** Mientras la barra está agarrada se engrosa y muestra un indicador redondo (thumb) en la posición del dedo. En reposo se ve igual que hoy.
- **La zona táctil crece** hasta al menos `UI_TOUCH_MIN` de alto, centrada en la barra y sin invadir los controles vecinos.
- **El mismo comportamiento en las dos barras:** la de Reproduciendo y la de la vista de letras usan la misma lógica. Mientras se arrastra en la vista de letras, las letras no siguen al dedo. Se reubican al soltar, cuando cambia la posición real.
- **La lógica del arrastre** (agarrar, seguir, soltar, convertir X en fracción) pasa a un módulo puro con tests en PC.
- **La spec se alinea con la pantalla:** a la derecha se muestra la duración total de la pista, no el tiempo restante.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `ui/now-playing`: el requirement "Seek control" pasa de "elapsed and remaining time" a tiempo transcurrido y duración total. Se agregan el arrastre con seek al soltar, la correspondencia exacta entre el punto tocado y la posición (incluido el final de la pista), la respuesta visual al tocar y la misma conducta en la vista de letras.

## Impact

- **`source/audio/audio.c`, `include/audio/audio.h`**: `Audio_Seek` recibe una fracción (`float` entre 0 y 1, acotada).
- **`source/audio/{flac,mp3,ogg,opus,wav,xm}.c` y sus headers**: cada `*_Seek` recibe la fracción y la multiplica por su largo total. Se quita la división por 450.
- **`source/menus/menu_audioplayer.c`**: las dos barras dibujan y atienden el toque con el módulo nuevo. Se quitan el escalado manual a 450 de la vista de letras y el `Touch_Position` de la barra principal.
- **Archivos nuevos**: `source/seek_bar.c`, `include/seek_bar.h` y `tests/test_seek_bar.c`. Se agregan a `CMakeLists.txt` y a `tests/Makefile`.
- **Versión y documentación**: `VITA_VERSION` a `03.60` y CHANGELOG.
- Sin dependencias, licencias ni assets nuevos. El CI solo suma el test nuevo.

## Rama

`change/fix-seek-bar`

## Fuera de alcance

- **Mostrar el tiempo restante** (`-1:23`). Se mantiene la duración total.
- **Seek con botones** (D-pad o gatillos para adelantar o retroceder). L y R siguen cambiando de pista.
- **Hacer seek en vivo durante el arrastre** (escuchar mientras se arrastra). El seek se hace solo al soltar.
- **Sincronizar el seek con el hilo de audio** (un lock entre la UI y el callback del decodificador). Como sigue habiendo un seek por gesto, el riesgo queda igual que hoy. Se registra en design.md.
- **Hacer que las letras sigan al dedo durante el arrastre.**
- **Cambiar el formato del tiempo** (horas, milisegundos).

## Notas de version

Versión objetivo: **v3.6.0** (minor: el arrastre de la barra es una funcionalidad nueva, junto con una corrección).

- Added: drag the progress bar to choose where to jump. The bar grows and shows a handle while you hold it, the elapsed time follows your finger, and the song jumps there when you let go.
- Fixed: touching the end of the progress bar now goes to the end of the song. Before, it stopped at about 95 %, and the filled part of the bar did not line up with your finger.
- Fixed: the progress bar is easier to hit: its touch area is taller.
