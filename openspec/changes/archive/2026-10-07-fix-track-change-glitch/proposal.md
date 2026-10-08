Rama: `change/fix-track-change-glitch`, creada desde `main`. Su PR apunta a `main`.

## Why

Al cambiar de canción aparecen a veces cuñas de color que cruzan la pantalla, y lo que se dibuja al final del fotograma (Up Next, la barra de hints, el nav rail) sale deformado. En consola pasó en **6 de 70 cambios con R**, tanto desde Biblioteca como desde Carpetas, y el contador de agotamiento del pool siguió en 0. `close-ui-contract` lo dejó registrado como limitación conocida y lo atribuyó al MSAA, porque con el suavizado apagado no aparecía. Esa atribución explica *cuándo* ocurre, pero no *qué* lo produce.

La hipótesis de este change es una carrera sobre el pool de vértices. vita2d tiene un solo pool por fotograma y lo reinicia en `vita2d_start_drawing` sin esperar a la GPU. Mientras todo va normal, el vblank frena a la CPU. Después del stall de un cambio de pista la cola de display queda vacía, la CPU deja de estar frenada y escribe el fotograma siguiente encima de vértices que la GPU todavía lee del primero. El MSAA alarga lo que tarda la GPU en cada fotograma, y por eso agranda la ventana sin ser la causa. Los síntomas encajan: triángulos con vértices corruptos, lo dibujado primero sano y lo dibujado al final roto, ocurrencia aleatoria y ningún agotamiento del pool.

La hipótesis no está confirmada todavía. Por eso el change empieza con un diagnóstico medible y solo aplica el arreglo general si los datos lo respaldan.

## What Changes

- **Etapa 1, diagnóstico (una sola compilación):**
  - **Punto único de inicio de fotograma.** Todas las pantallas empiezan a dibujar a través de una sola función. Esa función puede esperar a que la GPU termine el fotograma anterior antes de reiniciar el pool. La espera queda **apagada por defecto**, así la compilación reproduce el comportamiento actual y la línea base de 6/70 sigue siendo comparable.
  - **Modo "captura" en el overlay de debug** (L + R + SELECT pasa a recorrer apagado, estadísticas, sonda de glifos, captura). En ese modo:
    - Después de cada cambio de pista se copia en RAM el primer fotograma dibujado tras el cambio. Se copia con un fotograma de retraso, para no alterar la carrera que se quiere observar.
    - Se anotan los bytes del pool que usaron los dos primeros fotogramas tras el cambio. Si son distintos, la estructura del pool cambió entre ellos, que es lo que la hipótesis necesita.
    - **SELECT** guarda la última captura como imagen en `ux0:data/ElevenMPV/glitch/`, junto con un `.txt` con los bytes del pool y el estado de la espera.
    - **Arriba** (cruceta) enciende o apaga la espera del inicio de fotograma, así el A/B se hace sin recompilar.
- **Experimento en consola:** 70 cambios con R con la espera apagada y otros 70 con la espera encendida, con MSAA 4x.
- **Etapa 2, solo si el experimento confirma (0 o casi 0 de 70 con la espera encendida):** la espera pasa a ser incondicional en todas las pantallas y desaparece el interruptor. La captura sigue en el overlay como herramienta.
- **Si el experimento no confirma:** el change no aplica la etapa 2. Los resultados y las capturas quedan registrados en `tasks.md` y el diseño se revisa antes de seguir.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `ui/rendering`: el requirement "Ningún recurso gráfico se destruye con trabajo de dibujo pendiente" ya prohíbe *reutilizar* un recurso mientras haya dibujo pendiente sobre él. Se precisa que la memoria de geometría de un fotograma cuenta como recurso, y se agrega un escenario verificable de cambio de pista sin artefactos visuales.

## Fuera de alcance

- Cambiar o quitar el MSAA. Se conserva 4x, y la idea es justamente que deje de estar en conflicto con un cambio de pista limpio.
- Hacer asíncrono el desmontaje de audio para acortar el stall. Puede venir después, pero no es necesario si la espera resuelve el problema.
- Hacer un fork de vita2d para tener dos pools alternados. Sería más eficiente que esperar, pero vita2d usa su pool interno para rectángulos, texto y texturas, y no se puede alternar desde fuera.
- Guardar capturas automáticamente en cada cambio de pista. Escribir 2 MB en la tarjeta provocaría otro stall, y con él otra carrera.
- Editar el registro archivado de `close-ui-contract`. Es historia; la corrección de la atribución queda en este change.

## Notas de version

- Versión objetivo: **v3.4.3**, salto **patch** (corrección).
- Bullets para `CHANGELOG.md` (solo si se aplica la etapa 2):
  - Fixed colored streaks and garbled text that sometimes flashed across the screen right after changing tracks.

## Impact

- **Código C:** `source/ui_gpu.c` e `include/ui_gpu.h` (inicio de fotograma, modo captura del overlay), `source/menus/menu_audioplayer.c` (armar la captura al cambiar de pista) y cada pantalla que hoy llama a `vita2d_start_drawing`: `menu_audioplayer.c`, `menu_displayfiles.c`, `menu_library.c`, `menu_settings.c` y `library.c`.
- **Memoria:** un buffer de ~2 MB en el heap, reservado solo mientras el modo captura está visible.
- **Archivos en la tarjeta:** `ux0:data/ElevenMPV/glitch/`, solo cuando el usuario guarda una captura.
- **Rendimiento:** con la espera encendida, la CPU deja de grabar el fotograma siguiente mientras la GPU termina el anterior. Se mide que la app siga a 60 fps.
- **Specs:** delta de `ui/rendering`.
- **CI, dependencias, textos traducibles y assets:** sin cambios. Los textos del overlay son de debug y no se traducen, como los actuales.
