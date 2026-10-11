## 1. Seek por fracción

- [x] 1.1 Cambiar `Audio_Seek` a `SceUInt64 Audio_Seek(double fraction)`, acotada a [0, 1] en `source/audio/audio.c`. Cambiar los seis `*_Seek(double fraction)` (flac, mp3, ogg, opus, wav, xm, con sus headers) para que calculen `total * fraction` acotado a `total - 1` y quitar el 450 (Decisión 1). Adaptar las dos llamadas de `menu_audioplayer.c` para que pasen la fracción calculada con el ancho real de cada barra (`(x - bar_x) / bar_w`), todavía con el toque actual, y borrar el comentario del "old 450 px bar". Listo cuando `scripts/build.sh` compile sin warnings, `grep -rn 450 source/audio` no encuentre la división, y en la consola tocar el borde derecho de la barra principal deje la pista a menos de un par de segundos del final en FLAC, MP3, OGG, Opus, WAV y XM.

## 2. Lógica pura de la barra

- [x] 2.1 Crear `include/seek_bar.h` y `source/seek_bar.c` con `SeekBar_Geometry`, `SeekBar_State`, `SeekBar_Input`, `SeekBar_Update`, `SeekBar_FractionAt` y `SeekBar_DisplayFraction` (Decisión 2), sin vita2d ni SCE. Crear `tests/test_seek_bar.c` con todos los casos de "Estrategia de pruebas" de design.md y agregarlo a `BIN` y a las reglas de `tests/Makefile`. Agregar `source/seek_bar.c` a `CMakeLists.txt`. Listo cuando `make -C tests` pase con sanitizers y `scripts/build.sh` compile sin warnings.

## 3. Barras con arrastre

- [x] 3.1 Barra principal de Reproduciendo: reemplazar el `Touch_Position` de `SEEK_Y ± 12` por `SeekBar_Update` con la zona de 66 px (Decisión 3), probando después de `Menu_HandleTransportTouch`. En `COMMIT`, hacer pausa, `Audio_Seek(fraction)` y reanudación. Dibujar con `SeekBar_DisplayFraction`: barra de 8 px y thumb mientras está agarrada, el tiempo transcurrido siguiendo al dedo (Decisión 4). Reiniciar el estado al entrar, al alternar letras y al cambiar de pista (Decisión 5). Listo cuando `scripts/build.sh` compile y en la consola se cumplan estos escenarios de `ui/now-playing` con una pista larga de `ux0:/pruebas-eleven/flac/`:
  - "Seeking via touch";
  - "El borde derecho lleva al final";
  - "El borde izquierdo lleva al inicio";
  - "Arrastrar muestra el destino y salta al soltar";
  - "El dedo se sale de la barra durante el arrastre";
  - "Zona táctil más alta".

  Además, play, anterior, siguiente, barajar y repetir deben responder al tocarlos como antes.
- [x] 3.2 Barra de la vista de letras: usar el mismo módulo, con la zona acotada a la barra de hints, y mover el límite del arrastre de letras a `hit_top` (Decisión 3). En `COMMIT`, hacer el seek y llamar `Menu_LyricsSnap()`. Quitar `Menu_LyricsHandleSeek` con su escalado a 450. Listo cuando `scripts/build.sh` compile y en la consola se cumplan:
  - el escenario "La barra de la vista de letras se comporta igual" de `ui/now-playing`, con una pista con `.lrc` sincronizado;
  - que arrastrar las letras desde el centro de la pantalla siga funcionando como antes.

  Resultado (2026-10-11, consola, v03.60):
  - Borde derecho en FLAC, MP3, OGG, Opus, WAV y XM: todas llegaron al final y pasaron a la siguiente.
  - Barra principal: los seis escenarios de `ui/now-playing` se cumplieron. El transporte responde como antes y no se dispara al soltar encima.
  - Barra de letras: las letras no se mueven durante el arrastre y saltan a la línea nueva al soltar. El arrastre de letras sigue igual.
  - Con el relleno casi vacío no se ve el "grano" de la Decisión 4, así que no hace falta el ancho mínimo.

## 4. Versión y cierre

- [x] 4.1 Agregar los bullets de Notas de versión de `proposal.md` a `CHANGELOG.md` bajo `[Unreleased]` (en `### Added` y `### Fixed`) y subir `VITA_VERSION` a `03.60` en `CMakeLists.txt`. Listo cuando `scripts/build.sh --clean` compile sin warnings y el `.vpk` instalado encima de v3.5.1 muestre `03.60` y conserve la configuración y el índice.
- [x] 4.2 Push de `change/fix-seek-bar` y PR a `main`. Listo cuando el CI del PR esté en verde (build del `.vpk` y tests en PC, incluido `test_seek_bar`).
