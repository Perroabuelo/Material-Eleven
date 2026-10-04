## Context

El porqué está en proposal.md. Acá va el estado del código que condiciona el cómo.

**Cómo se navega hoy.** `main()` llama una vez a `Menu_DisplayFiles()` y ninguna pantalla vuelve. Cada pantalla tiene un `while (SCE_TRUE)` y, para cambiar de pantalla, llama a la otra y después hace `return`. Ese `return` no se ejecuta nunca, porque la pantalla llamada tampoco vuelve:

```
  main()
   +-- Menu_DisplayFiles()                 Carpetas
        +-- Menu_DisplayLibrary()          tap Biblioteca
             +-- Menu_LibraryPlaySelected()
                  +-- Menu_PlayQueued()
                       +-- Menu_RunNowPlayingLoop()      Reproduciendo
                            +-- Menu_DisplayLibrary()    volver
                                 +-- Menu_DisplaySettings()
                                      +-- ... un marco mas por cada salto
```

Hay cuatro tipos de salto, y todos tienen esta forma:

| Desde | Disparador | Hoy llama a |
|---|---|---|
| Cualquier pantalla | toque en el nav rail | `Menu_Display*` / `Menu_ShowNowPlaying` |
| Carpetas, Biblioteca | SELECT | `Menu_DisplaySettings` |
| Ajustes | volver | `Menu_DisplayFiles` |
| Reproduciendo | volver | `Menu_DisplayLibrary` o `Menu_DisplayFiles` según `playback_origin` |
| Carpetas | confirmar sobre un archivo | `Menu_HandleControls` → `Dirbrowse_OpenFile` → `Menu_PlayAudio` → lazo de Reproduciendo |
| Biblioteca | confirmar sobre una pista | `Menu_LibraryPlaySelected` → `Menu_PlayQueued` → lazo de Reproduciendo |

**El estado de cada pantalla ya vive fuera de su lazo.** Carpetas lo guarda en `cwd` y `dirbrowse.c`; la Biblioteca en los `static` de `menu_library.c`; Reproduciendo en los `static` de `menu_audioplayer.c`. Hoy cada salto ya entra a la pantalla con una llamada nueva, así que entrar a una pantalla desde un despachador no cambia lo que se ve.

**Lazos internos que sí vuelven.** `Menu_PickFolder`, `Menu_PromptFilter`/`Menu_AbandonFilterDialog` y el escaneo de la biblioteca corren su propio lazo de dibujo, pero terminan y vuelven a quien los llamó. Están acotados y quedan como están.

**El overlay de debug no ve la pila.** `ui_gpu.c` muestra la memoria libre de usuario y de CDRAM. La pila del hilo principal se reserva entera al crear el hilo, así que esas cifras no se mueven aunque la pila se esté llenando.

## Goals / Non-Goals

**Goals:**
- Profundidad de pila constante entre pantallas, verificable en la consola.
- Que el comportamiento visible sea el mismo: mismas pantallas, mismos botones, misma vuelta al origen.
- Tocar lo mínimo en cada pantalla: el cuerpo de cada lazo no cambia; solo cambia qué se hace al decidir el salto.

**Non-Goals:**
- Una pila de historial de navegación.
- Cambiar los lazos internos que ya vuelven (selector de carpeta, filtro, escaneo).
- Que la cola avance en segundo plano fuera de Reproduciendo.

## Decisions

### 1. Cada pantalla devuelve la siguiente; `main` despacha

`Menu_DisplayFiles`, `Menu_DisplayLibrary`, `Menu_DisplaySettings` y `Menu_ShowNowPlaying` pasan a devolver `UI_Screen`. Donde hoy hacen `Menu_X(); return;`, pasan a hacer `return UI_SCREEN_X;`. `main.c` reemplaza la llamada a `Menu_DisplayFiles()` por un lazo:

```
  UI_Screen next = UI_SCREEN_FOLDERS;
  while (next != UI_SCREEN_NONE) {
      Touch_Reset();
      switch (next) {
          FOLDERS     -> next = Menu_DisplayFiles()
          LIBRARY     -> next = Menu_DisplayLibrary()
          SETTINGS    -> next = Menu_DisplaySettings()
          NOW_PLAYING -> next = Audio_HasTrack() ? Menu_ShowNowPlaying() : UI_SCREEN_FOLDERS
      }
  }
```

- `UI_SCREEN_NONE` queda como salida del lazo. Hoy ninguna pantalla lo devuelve, igual que hoy ninguna vuelve a `main`, pero así la limpieza que sigue en `main` vuelve a tener una forma de alcanzarse.
- `Touch_Reset()` pasa a hacerse en un solo lugar, en cada cambio. Hoy lo hacen la Biblioteca y Reproduciendo, pero no Carpetas ni Ajustes. Centralizarlo evita que un toque que cambió de pantalla siga contando en la pantalla nueva.
- La guarda de `Audio_HasTrack()` ya existe en cada pantalla antes de ir a Reproduciendo. El despachador la repite como defensa.

- **Alternativa descartada: una pila de pantallas con push/pop.** Resuelve otra cosa (historial). Acá solo hace falta que la profundidad no crezca.
- **Alternativa descartada: `longjmp` al lazo de `main`.** Corta la pila sin tocar las pantallas, pero se salta cualquier limpieza pendiente y es frágil con `-Werror` y el optimizador. No vale el ahorro.

### 2. Pedir Reproduciendo desde el fondo de la pila de llamadas: un pedido diferido

Reproducir no nace en el lazo de la pantalla, sino varias llamadas más abajo (`Menu_HandleControls` → `Dirbrowse_OpenFile` → `Menu_PlayAudio`). Para no cambiar la firma de toda esa cadena, se agrega un módulo puro mínimo, `nav_request.c`:

```
  void      NavRequest_Set(UI_Screen screen);   // el ultimo pedido gana
  UI_Screen NavRequest_Take(void);              // lo devuelve y lo borra; NONE si no hay
```

- `Menu_PlayAudio` y `Menu_PlayQueued` cargan la pista como hoy, fijan `playback_origin`, ponen la cola en la pista elegida y, en vez de entrar al lazo de Reproduciendo, llaman a `NavRequest_Set(UI_SCREEN_NOW_PLAYING)`. Siguen devolviendo `SCE_FALSE` si la pista no abre, así que el aviso de la Biblioteca sigue funcionando.
- Carpetas y Biblioteca, después de procesar los controles de cada fotograma, hacen `UI_Screen r = NavRequest_Take(); if (r != UI_SCREEN_NONE) return r;`.
- El despachador hace `NavRequest_Take()` antes de abrir cada pantalla y descarta el resultado, para que un pedido que nadie consumió no se cuele en la pantalla siguiente.

`UI_Screen` vive hoy en `ui_theme.h`, que incluye vita2d. Para que `nav_request.c` compile en PC, el `enum` pasa a un encabezado propio sin dependencias, `include/ui_screen.h`, que `ui_theme.h` incluye. Ninguno de los que usan `UI_Screen` hoy tiene que cambiar su `#include`.

- **Alternativa descartada: propagar un valor de retorno por toda la cadena.** Obliga a cambiar `Menu_HandleControls`, `Dirbrowse_OpenFile` y `Menu_LibraryPlaySelected`, que hoy son `void`, solo para pasar un dato hacia arriba.
- **Alternativa descartada: una variable global suelta.** Funciona igual, pero el módulo de cinco líneas se puede probar en PC y deja claro quién lo usa.

### 3. Reproduciendo devuelve su origen cuando se queda sin pista

Si `track_failed` está puesto (ninguna pista de la cola abrió), `Menu_RunNowPlayingLoop` hoy hace `return` a quien lo llamó, que puede ser cualquier pantalla según la historia de llamadas. Ahora devuelve `playback_origin`, lo mismo que el botón de volver. La rama de volver deja de llamar a `Menu_DisplayLibrary()`/`Menu_DisplayFiles()` y devuelve `playback_origin` directamente.

### 4. El overlay de debug muestra la pila usada

`main()` registra, al arrancar, la dirección de una variable local (`UI_Debug_MarkStackBase`). `UI_Debug_Draw`, que ya se llama desde el lazo de cada pantalla, toma la dirección de una local suya y muestra la diferencia en bytes como "pila", junto con el máximo visto en la sesión. Con el despachador, el valor en una misma pantalla tiene que ser siempre el mismo; antes de este cambio crece con cada salto. Así el problema se puede ver en la consola antes y después del cambio.

- **Alternativa descartada: preguntarle al kernel por la pila del hilo (`sceKernelGetThreadInfo`).** Da el tamaño reservado, no el uso actual.

### 5. El listado de Carpetas se libera con un lazo

`Dirbrowse_RecursiveFree` hace una llamada por nodo, hasta 1025 marcos. Pasa a ser un lazo que avanza por `next` liberando cada nodo. Es el mismo resultado sin depender de la pila.

## Risks / Trade-offs

- **[Algún camino de salto queda sin convertir]** → Una llamada directa que quede (por ejemplo, un `Menu_DisplayFiles()` dentro de otra pantalla) vuelve a anidar sin que nada falle. Mitigación: al terminar, `grep` de `Menu_Display\|Menu_ShowNowPlaying` en `source/menus/` y en `source/dirbrowse.c` no debe mostrar llamadas fuera de `main.c`. Además, los scenarios de la spec lo detectan con el overlay.
- **[Un pedido de Reproduciendo se consume tarde]** → Si una pantalla no revisa `NavRequest_Take()` después de una acción que reproduce, la app se quedaría en esa pantalla con la música sonando. Mitigación: solo Carpetas y Biblioteca reproducen, y en las dos el chequeo va justo después de los controles. La prueba en consola reproduce desde las dos.
- **[Ida y vuelta sin estado guardado]** → Si alguna pantalla inicializa estado al entrar, se reiniciaría en cada vuelta. No es nuevo: hoy cada salto ya entra con una llamada nueva. No cambia lo que el usuario ve.
- **[Mini reproductor y fallo de toda la cola]** → Si desde el mini reproductor se pasa de pista y ninguna abre, `track_failed` queda puesto hasta que se entra a Reproduciendo, que vuelve en el acto al origen. Es lo mismo que pasa hoy, ahora con un destino definido.

## Migration Plan

No hay datos que migrar. Para volver atrás basta revertir los commits.

## Estrategia de pruebas

- **Compilación**: `scripts/build.sh` sin errores ni avisos con `-Wall -Werror`.
- **Pruebas en PC**: `tests/test_nav_request.c` cubre el módulo nuevo. Sin pedido, `Take` devuelve `UI_SCREEN_NONE`. `Take` devuelve lo pedido y lo borra. Si hay dos pedidos, gana el último. Se agrega a `tests/Makefile`, y `make -C tests` debe pasar completo.
- **Pruebas en consola**: los scenarios de `specs/ui/nav-shell` con el overlay de debug. La línea de pila del overlay se agrega primero, antes de tocar la navegación. Así se puede comprobar en la consola que hoy la pila crece con cada salto. Después se repite la prueba con el despachador, para confirmar que no crece. Además se repasan los pasos de verificación de `fix-back-to-origin`, para confirmar que la vuelta al origen no cambió.

## CI

Sin cambios en el pipeline. El CI no corre las pruebas de PC, y este cambio no lo modifica.

## Código de terceros

No se incorpora código ni assets de terceros.
