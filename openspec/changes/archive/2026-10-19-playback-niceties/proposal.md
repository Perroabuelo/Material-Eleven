## Why

Tres comportamientos que cualquier otro reproductor tiene y que en Material-Eleven faltan: retroceder siempre salta a la pista anterior aunque la canción vaya por la mitad, no hay forma de dejar la música sonando para dormirse sin que siga toda la noche, y pausar o reanudar corta el sonido en seco, con un clic audible en auriculares. Son cambios chicos, se notan en el uso diario y no dependen de reestructurar el camino del audio.

## What Changes

- **Retroceder reinicia la pista si ya pasaron más de 3 segundos.** Si la pista en curso lleva más de 3 segundos, retroceder la vuelve a empezar desde 0:00. Si lleva 3 segundos o menos, retroceder pasa a la pista anterior del orden vigente, como hoy. El umbral es fijo y vale para todas las superficies (L en Reproduciendo, el botón de anterior en pantalla y el del mini reproductor), también con la repetición encendida. Reiniciar no cambia la pausa: una pista en pausa vuelve a 0:00 y sigue en pausa.
- **Temporizador para dormir, desde Reproduciendo.** SELECT abre en Reproduciendo un menú con las opciones Apagado, 15, 30, 45, 60 y 90 minutos, y "Al terminar la pista". Al vencer un tiempo, la música baja de volumen durante unos 10 segundos y se pausa. Con la reproducción en pausa, la consola vuelve a poder suspenderse sola, como ya define `playback/screen-off`. Con "Al terminar la pista", cuando la pista en curso termina sola, la cola pasa a la siguiente pista, como siempre, pero la deja cargada en pausa en 0:00. Elegir otra opción reemplaza la anterior, y elegir Apagado lo cancela. El temporizador no se recuerda entre arranques.
- **Indicador del temporizador.** Mientras hay un temporizador activo, la barra de estado (Reproduciendo, Carpetas y Biblioteca) muestra una luna con los minutos que faltan, o con "Fin de pista" en el modo de fin de pista.
- **Fundido corto al pausar y reanudar.** Pausar baja el volumen a cero en unos 100 ms antes de detener el sonido, y reanudar lo sube desde cero en el mismo tiempo. Siempre está activo y no se configura. El estado de pausa que muestran la pantalla y la leyenda cambia en el acto, sin esperar al fundido.
- **Leyenda.** Reproduciendo anuncia SELECT con la acción "Dormir" ("Sleep"). Para que entre, la leyenda se compacta: L y R dicen "<< >>", D-pad arriba y abajo van juntos como "Vistas" ("Views") y Triángulo dice "Mezclar". En la vista de visualizador no entra y se cae desde el final, como START.
- **Log de desarrollo.** Una línea `track` `event=restart` al reiniciar una pista, una categoría `sleep` (`event=set` y `event=expire`, con `mode`) y dos campos nuevos al final de `nowplaying`: `menu` y `sleep`.

## Capabilities

### New Capabilities

- `playback/sleep-timer`: el temporizador para dormir. Sus opciones, la cuenta regresiva, el fundido y la pausa al vencer, el modo de fin de pista, el indicador en la barra de estado, que siga contando en cualquier pantalla y con la pantalla apagada, y que no se recuerde entre arranques.
- `playback/pause-fade`: el fundido corto al pausar y al reanudar, y que buscar con la barra de progreso no lo dispare.

### Modified Capabilities

- `playback/queue`: el requirement "Avanzar y retroceder por el orden vigente". Retroceder reinicia la pista en curso cuando ya pasaron más de 3 segundos, y solo pasa a la anterior si pasaron 3 segundos o menos.
- `ui/now-playing`: se agrega cómo se abre el menú del temporizador con SELECT, cómo se maneja, y su entrada en la leyenda. Se modifica el requirement "La leyenda de Reproduciendo anuncia las letras": D-pad arriba y abajo pasan a una sola entrada "Vistas", y L y R a "<< >>".
- `dev/debug-log`: se agregan la línea `track` `event=restart`, la categoría `sleep` y los campos `menu` y `sleep` al final de `nowplaying`.

## Impact

- **Código nuevo, puro y probado en PC:** el temporizador (opciones, cuenta regresiva, ganancia del fundido final, minutos restantes, fin de pista), la rampa de ganancia del fundido de pausa y la regla de los 3 segundos. Tests nuevos en `tests/`.
- **Audio:** `source/audio/audio.c` (`Audio_Decode` aplica la rampa de pausa y la ganancia del temporizador, la pausa del usuario pasa a ser una intención que el hilo de audio cumple al terminar el fundido, una pausa inmediata sin fundido para buscar, y abrir una pista ya en pausa) e `include/audio/audio.h`.
- **Reproduciendo:** `source/menus/menu_audioplayer.c` (un solo camino de retroceder para L, el transporte en pantalla y el mini reproductor; el menú del temporizador con SELECT; la leyenda; los campos nuevos de `nowplaying`).
- **Otras superficies:** `source/mini_player.c` (sin cambios de lógica, ya llama a `Music_Previous`), `source/utils.c` (el temporizador avanza una vez por fotograma junto a `Music_Update`), `source/status_bar.c` (el indicador) e `include/lang_strings.h` (textos en inglés y español). La leyenda reusa el chip de SELECT que ya existe.
- **Documentación:** README (retroceder, el temporizador y el fundido) y CHANGELOG.
- Sin dependencias nuevas. Sin cambios en `config.cfg`: sigue en la versión 4.

## Rama

`change/playback-niceties`, creada desde `main`. Es la primera de una serie de cambios apilados (`resume-session`, `replaygain`, `queue-actions`, `audio-mixer`), que se crean cada uno desde la rama del anterior.

## Fuera de alcance

- **Umbral de retroceso configurable.** Son 3 segundos fijos.
- **Recordar el temporizador entre arranques**, o elegir un tiempo personalizado distinto de los de la lista.
- **Abrir el temporizador desde Ajustes, desde otras pantallas o tocando la pantalla.** Se abre con SELECT en Reproduciendo. Las opciones del menú sí se pueden tocar.
- **Apagar o suspender la consola al vencer.** El temporizador pausa, y la suspensión la hace la consola sola por inactividad.
- **Configurar o desactivar el fundido de pausa**, o fundir al cambiar de pista, al buscar o al terminar la cola. Los fundidos entre pistas son parte de `audio-mixer`.
- **Adelantar o retroceder dentro de la pista con L y R mantenidos.**

## Notas de version

Versión objetivo: **v4.5.0** (minor: funcionalidad nueva). En la consola se ve como `04.50`.

- Previous now restarts the song when it has played for more than 3 seconds, and goes to the previous song otherwise. This works the same from L, the on-screen button and the mini player.
- Sleep timer: press SELECT in Now Playing to stop the music after 15, 30, 45, 60 or 90 minutes, or at the end of the current song. The music fades out over a few seconds and pauses.
- While a sleep timer is set, the status bar shows a moon with the minutes left.
- Pausing and resuming now fade the sound out and in quickly, instead of cutting it with a click.
