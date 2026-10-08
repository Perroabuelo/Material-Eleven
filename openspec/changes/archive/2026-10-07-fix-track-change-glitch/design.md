## Context

Ver `proposal.md` (Why) para el síntoma y la hipótesis. Hechos del código que condicionan el diseño:

- vita2d tiene **un solo** pool de vértices por fotograma (2 MB, `UI_VERTEX_POOL_SIZE` en `source/main.c`). `vita2d_start_drawing` lo reinicia a 0 y no espera a la GPU. vita2d usa ese pool internamente para rectángulos, círculos, texto y texturas, y la app lo usa a través de `UI_GpuPoolAlloc` para sus formas vectoriales.
- Hay **11 llamadas** a `vita2d_start_drawing`, repartidas en `menu_audioplayer.c`, `menu_displayfiles.c`, `menu_library.c`, `menu_settings.c` y `library.c`. Cada pantalla tiene su propio lazo.
- `Music_HandleNext` (`source/menus/menu_audioplayer.c`) es la única ruta de cambio de pista. La usan L/R, el toque en los botones, el fin natural de la pista y el mini reproductor (`Music_Next`/`Music_Previous`). Empieza con `vita2d_wait_rendering_done()` (desde `03cd9ae`), así que el fotograma *previo* al stall ya está protegido. Lo que no está protegido son los fotogramas *posteriores*.
- Hay dos elementos que cambian de estructura justo después del cambio de pista. La barra de progreso con posición 0 no dibuja nada (`UI_DrawRoundedRect` con `w = 0`) y con posición mayor que 0 dibuja un rectángulo y cuatro abanicos de arco. La carátula de la segunda fila de Up Next llega un fotograma después que la primera. Con el primero basta para que dos fotogramas seguidos usen el pool de forma distinta, y ya existía antes de Up Next.
- El overlay de debug (`source/ui_gpu.c`) ya tiene un ciclo de modos con L + R + SELECT, se actualiza fuera de la escena (`UI_Debug_Update`, después del swap) y dibuja último dentro de ella (`UI_Debug_Draw`).

## Goals / Non-Goals

**Goals:**
- Confirmar o descartar la hipótesis con un A/B **dentro de una misma compilación**, contra la línea base medida de 6/70.
- Obtener evidencia directa del mecanismo: una imagen del fotograma roto y los bytes de pool de los fotogramas involucrados.
- Si se confirma, que la garantía sea incondicional y pase por un solo punto, igual que `UI_GpuFreeTexture`.

**Non-Goals:**
- Detectar automáticamente un fotograma corrupto. Lo decide el ojo del usuario y SELECT.
- Acortar el stall del cambio de pista.

## Decisions

### 1. Un solo punto de inicio de fotograma: `UI_GpuBeginFrame()`

Las 11 llamadas a `vita2d_start_drawing()` pasan a `UI_GpuBeginFrame()` (en `ui_gpu.c`). En la etapa 1 hace esto:

```
UI_GpuBeginFrame:
    si frame_sync encendido -> vita2d_wait_rendering_done()
    vita2d_start_drawing()
```

En la etapa 2 la espera pasa a ser incondicional y `frame_sync` desaparece.

**Por qué en todas las pantallas y no solo en Now Playing:** la carrera depende de cualquier stall seguido de un reinicio del pool, no del cambio de pista en particular. Un escaneo de biblioteca o la vuelta de la pantalla apagada producen la misma situación. Además, `ui_gpu.h` ya fija la lección de `ac8f53d`/`f3d908e`: la sincronización va en un punto de paso único y no repartida por convención.

**Alternativas descartadas:**
- *Esperar solo durante unos fotogramas después de `Music_HandleNext`.* Es más barato, pero ataca un caso y deja los demás stalls con la misma carrera. Además depende de adivinar cuántos fotogramas dura la ventana.
- *Dos pools alternados.* Requiere un fork de vita2d (ver Fuera de alcance).
- *Bajar o quitar el MSAA.* Achica la ventana pero no la cierra, y contradice el requirement de bordes suavizados.

**Costo de esperar:** la CPU deja de grabar el fotograma N+1 mientras la GPU termina el N. La app hoy va capada a 60 fps por el vblank con margen (medido en `close-ui-contract`, tarea 2.2), así que se espera que no se note. Se mide igual (ver Pruebas).

### 2. Captura con un fotograma de retraso

Si la captura esperara a la GPU para copiar el fotograma recién dibujado, impediría justamente la carrera que quiere observar, igual que el arreglo. Por eso copia el fotograma **anterior**, cuando ya está dibujado y la carrera ya pudo ocurrir:

```
 cambio de pista            -> UI_Debug_ArmCapture()  (al final de Music_HandleNext)
 fotograma N+1  Draw        -> guarda fb(N+1) = vita2d_get_current_fb(),
                               pool_used(N+1) = pool_size - vita2d_pool_free_space()
                swap
 fotograma N+2  (la CPU escribe el pool: aqui ocurre la carrera con N+1)
                Draw        -> pool_used(N+2)
                swap
                Update      -> vita2d_wait_rendering_done()
                               memcpy(buffer, fb(N+1), 960*544*4)
                               captura lista
```

- El puntero del framebuffer se toma **dentro** del fotograma (`UI_Debug_Draw`), y no se calcula por índice de buffer, para no depender de cuántos buffers de display use vita2d. El buffer de N+1 no se reutiliza antes de que empiece N+3, y la copia termina antes.
- La espera solo afecta a N+2, que queda protegido de una carrera con N+3. N+1, que es el candidato principal porque es el primer fotograma con la cola de display vacía, se observa sin alterar.
- `pool_used` se mide en `UI_Debug_Draw`, que es lo último que se dibuja, así que cubre todo el fotograma salvo el panel del propio overlay. En el modo captura el panel se dibuja igual en N+1 y en N+2, por lo que no afecta la comparación.

### 3. Controles del modo captura

- **L + R + SELECT** sigue recorriendo los modos: apagado, estadísticas, sonda de glifos, captura.
- **SELECT** (solo) guarda la última captura. **Arriba** en la cruceta enciende o apaga `frame_sync`.
- Ninguno de los dos se usa en Now Playing, que es donde se hace la prueba. En Carpetas y Biblioteca, SELECT y Arriba tienen su propia función y también se ejecutarían. Se acepta porque es una herramienta de debug y el panel lo advierte.
- SELECT guarda solo con flanco de subida y sin L ni R presionados, para no confundirse con el combo del overlay.

El panel del modo captura muestra la espera (`SYNC encendido/apagado`), el estado de la captura (`sin captura`, `lista`, `guardada glitch_NNN`, `SIN MEMORIA`, `ERROR AL GUARDAR`), los `pool_used` de N+1 y N+2 con su diferencia, y los controles.

### 4. Formato de archivo

`ux0:data/ElevenMPV/glitch/glitch_NNN.bmp` es un BMP de 32 bits, 960x544, con las filas invertidas y los canales convertidos de ABGR a BGRA. Al lado va `glitch_NNN.txt`, con `pool_used` de N+1 y N+2, el estado de `frame_sync` y el modo de suavizado. NNN es el primer número libre a partir de 000, buscado con `sceIoGetstat`. Se elige BMP porque no necesita ninguna librería ni compresión, y se abre en cualquier visor del PC.

Toda la lógica de conversión de píxeles y de armado de la cabecera BMP vive en una función pura (sin vita2d ni SCE), testeable en PC.

### 5. Recursos: quién libera qué

| Recurso | Lo reserva | Lo libera | Si algo falla a mitad |
|---|---|---|---|
| Buffer de captura (~2 MB, `malloc`) | Al **entrar** al modo captura | Al **salir** del modo captura (siguiente L + R + SELECT) y en `UI_Debug_Free` al cerrar la app | Si `malloc` falla, el panel muestra `SIN MEMORIA`, la captura no se arma y no hay nada que liberar |
| Buffer de fila para escribir el BMP (960*4 B) | Al guardar | Al terminar de guardar, en todos los caminos | Si falla, se cierra el archivo y se informa `ERROR AL GUARDAR` |
| Archivo `.bmp`/`.txt` (`sceIoOpen`) | Al guardar | `sceIoClose` en todos los caminos | Una escritura corta cierra el archivo y muestra `ERROR AL GUARDAR`. El archivo parcial queda en la tarjeta y se puede borrar a mano |

No se crean hilos. La escritura es sincrónica y ocurre fuera de la escena, solo cuando el usuario aprieta SELECT, así que el stall que provoca no contamina una captura que esté en curso: la captura ya terminó.

## Estrategia de pruebas

- **Compilación:** limpia con `-Wall -Werror`, en local y en CI.
- **Tests en PC:** test nuevo de la función pura que arma el BMP. Comprueba la cabecera (tamaños, offset, 32 bpp), la inversión de filas y la conversión de canales sobre una imagen chica conocida. Corre con ASan y UBSan como el resto.
- **Consola, etapa 1 (experimento):**
  1. Con el modo captura abierto y `SYNC apagado`, hacer 70 cambios con R en Now Playing desde Biblioteca. Contar cuántos muestran el glitch y guardar con SELECT al menos dos capturas de los que lo muestran.
  2. Con `SYNC encendido`, otros 70 cambios con R, contando de la misma forma.
  3. Copiar las capturas al PC y anotar si en cada una lo roto empieza después de lo que dibujó el primer elemento que cambia de estructura, y si los `pool_used` de N+1 y N+2 difieren.
  4. Anotar la línea HEAP del overlay antes de entrar al modo captura, con el modo abierto y después de salir, para verificar que los ~2 MB vuelven.
- **Criterio de decisión:** con la espera encendida, **0 de 70** confirma la hipótesis (con la tasa base de 8.5%, no ver ninguno por azar tiene ~0.2% de probabilidad). 1 o 2 de 70 indica que hay otra causa además de esta. Unos 6 de 70 la descarta.
- **Consola, etapa 2:** repetir 70 cambios con R con la espera ya incondicional, recorrer las cuatro pantallas para confirmar que siguen a 60 fps con el mismo método de `close-ui-contract` 2.2, y repetir el escenario existente de cambio repetido de track con y sin carátula.

## CI

Sin cambios en el pipeline. El test nuevo entra en `tests/Makefile` y el job existente lo corre.

## Terceros

No se incorpora código ni assets de terceros.

## Risks / Trade-offs

- [La espera cuesta rendimiento en alguna pantalla] → Se mide en la etapa 2 en las cuatro pantallas. Si alguna baja de 60, la alternativa es limitar la espera a los fotogramas posteriores a un stall, que la etapa 1 deja ya instrumentado.
- [La captura no llega a tomar el fotograma roto porque el glitch cae en N+2 y no en N+1] → La espera de la captura protege N+2, así que con la captura activa el glitch solo podría verse en N+1. Si con la captura activa la tasa con `SYNC apagado` cae mucho por debajo de 6/70, eso mismo es evidencia de que N+2 participa. Se anota y se compara con una tanda sin el modo captura.
- [Los ~2 MB del buffer no entran en el heap] → El modo muestra `SIN MEMORIA` y la prueba sigue sin captura. El A/B no depende de la captura.
- [SELECT o Arriba disparan acciones en otras pantallas mientras el modo está abierto] → Es una herramienta de debug, el panel lo advierte y la prueba se hace en Now Playing.

## Migration Plan

No hay datos ni configuración que migrar. La etapa 2 reemplaza el interruptor por la espera incondicional en un solo commit, así que para volver atrás basta con revertir ese commit.
