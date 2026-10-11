## Context

Ver proposal.md, Why. Estado actual relevante:

- **Fin de pista.** Solo `Menu_RunNowPlayingLoop` (`menu_audioplayer.c:815`) comprueba, después de `vita2d_swap_buffers`, `if (!playing && Queue_Count() != 0)` y llama `Music_HandleNext(SCE_TRUE, repeat)`. `playing` se pone en falso cuando el decodificador llega al final. Mientras tanto `Audio_Decode` escribe ceros, así que la pista queda cargada, sin pausa y en silencio. La pausa no toca `playing`.
- **`Music_HandleNext(forward, replay)`** desmonta la pista en curso (`Audio_Stop`, `Music_FreeCurrentTrack`, `Audio_Term`) y arma la siguiente:
  - con `replay`, toma la pista en curso con `Queue_PeekAhead(0, ...)`; si no, avanza con `Queue_Advance(forward)`;
  - prueba `Menu_InitMusic` como mucho `Queue_Count()` veces, avanzando en la misma dirección;
  - si ninguna abre, pone `track_failed`, que Reproduciendo lee al principio de cada fotograma para salir a `playback_origin`. `track_failed` se limpia al entrar a Reproduciendo.

  La usan también L y R, el transporte táctil y el mini reproductor (`Music_Next`, `Music_Previous`).
- **La repetición** es un `static SceBool repeat` de `menu_audioplayer.c`. Solo se cambia en Reproduciendo, pero vale para toda la app.
- **Un cambio de pista toca la GPU:** carátula a textura y acento. `Menu_InitMusic` además mide texto y diagrama la letra. Hoy ocurre siempre en el hilo principal, entre fotogramas.
- **`Utils_ReadControls()`** (`utils.c`) se llama una vez por fotograma desde Carpetas, Biblioteca, Ajustes y Reproduciendo, desde el bucle del diálogo de búsqueda (`menu_displayfiles.c:201`) y desde los bucles de escaneo de `library.c`. En Reproduciendo se llama justo después de la comprobación de fin de pista.
- **Módulos puros existentes** (`screen_off`, `nav_request`, `accent`...): sin vita2d ni headers de SCE, probados en PC con `make -C tests` (ASan + UBSan).

## Goals / Non-Goals

**Goals:**
- Un único punto atiende el fin de pista, en el hilo principal y entre fotogramas, como hoy.
- La decisión "repetir, avanzar, saltar las que no abren, una vuelta como máximo" es código puro con tests, incluido el caso en que nada abre.
- L, R, el transporte y el mini reproductor siguen pasando por `Music_HandleNext` con el mismo comportamiento.

**Non-Goals:**
- Un hilo propio para el avance de la cola.
- Cambiar qué pista elige la cola, el barajado o dónde vive la repetición.

## Decisions

### 1. `Music_Update()` en `Utils_ReadControls()`

`menu_audioplayer.c` expone `void Music_Update(void)`: si hay pista cargada (`Audio_HasTrack()`), terminó (`!playing`) y la cola no está vacía, llama `Music_HandleNext(SCE_TRUE, repeat)`. `Utils_ReadControls()` la llama al principio, antes de leer el pad. Reproduciendo borra su comprobación, porque `Utils_ReadControls()` ya corre en el mismo punto de su bucle: después de `vita2d_swap_buffers` y antes de leer la entrada.

La condición `Audio_HasTrack()` no está en la comprobación actual y hace falta. Cuando nada abre, `Audio_Term` ya corrió y `playing` queda en falso con la cola llena. Sin esa condición, `Music_Update` volvería a probar la cola entera en cada fotograma.

Llamarla desde `Utils_ReadControls()` cubre también el diálogo de búsqueda y los escaneos de la biblioteca. En los dos casos el bucle llama `Utils_ReadControls()` fuera del dibujo, así que el cambio de pista cae entre fotogramas, como en las pantallas.

*Alternativas descartadas:*
- **Una llamada explícita en cada bucle de pantalla.** Son cuatro pantallas más el diálogo y tres bucles de escaneo: ocho lugares donde olvidarse de algo, y el defecto actual es justamente eso.
- **Un hilo que avance la cola.** El cambio de pista crea texturas, cambia el acento y diagrama la letra mientras otra parte dibuja. Exigiría locks sobre el estado de la interfaz.

### 2. La decisión sale a `track_advance`, con las acciones inyectadas

```c
// include/track_advance.h - sin vita2d ni SCE
typedef struct {
	int (*open)(void *ctx, const char *path);           // 1 si abrió
	const char *(*advance)(void *ctx, int forward);     // siguiente del plan, o NULL
	const char *(*current)(void *ctx);                  // la que suena, o NULL
	void *ctx;
} TrackAdvance_Ops;

// Devuelve 1 si alguna pista abrió, 0 si ninguna.
int TrackAdvance_Run(const TrackAdvance_Ops *ops, int count, int forward, int replay);
```

La función hace exactamente lo que hoy hace el cuerpo de `Music_HandleNext` después del desmontaje:
- toma la pista inicial (`current` si es repetición, `advance` si no);
- prueba `open` como mucho `count` veces, avanzando en la misma dirección;
- devuelve el resultado.

`Music_HandleNext` conserva el desmontaje, `UI_Debug_ArmCapture` y el `track_failed`. Le pasa adaptadores sobre `Menu_InitMusic`, `Queue_Advance` y `Queue_PeekAhead`. En PC, los dobles abren o fallan según una tabla, y así se cubren los casos que la consola no puede provocar.

Los tipos son `int` y `const char *`, no `SceBool`, para que el header compile en PC sin headers de SCE, como `screen_off.h`.

*Alternativa descartada: probar `Music_HandleNext` tal como está.* Arrastra `Audio_*`, vita2d y la cola real a los tests.

### 3. Qué pasa cuando nada abre, según la pantalla

`track_failed` sigue siendo la señal para Reproduciendo, que sale a `playback_origin` como hoy. Fuera de Reproduciendo no hay nada más que hacer: `Audio_Term` ya corrió, `Audio_HasTrack()` es falso y el mini reproductor deja de dibujarse. Por eso la pantalla se queda donde está, como pide el escenario "Ninguna pista abre fuera de Reproduciendo".

Un `track_failed` puesto fuera de Reproduciendo queda en verdadero, pero no tiene efecto: Reproduciendo lo limpia al entrar (`Menu_RunNowPlayingLoop`). Además, sin pista no se entra a Reproduciendo (`Menu_ShowNowPlaying` vuelve a Carpetas).

### Estrategia de pruebas

- **Compilación** limpia con `-Wall -Werror` (`scripts/build.sh`).
- **Tests en PC** (`tests/test_track_advance.c`, ASan + UBSan):
  - avanzar abre la siguiente;
  - repetir reabre la que suena sin avanzar;
  - retroceder va hacia atrás;
  - se salta una pista que no abre y se sigue en la misma dirección;
  - cola de una pista, con y sin repetición;
  - ninguna abre: devuelve 0 después de exactamente `count` intentos;
  - `advance` que devuelve NULL no se pasa a `open`;
  - cola vacía (`count` 0): no se intenta nada.
- **Consola,** según los escenarios de `playback/queue`, con las pistas cortas de `ux0:/pruebas-eleven/formatos/`:
  - "La cola sigue en Carpetas";
  - "La cola sigue en la Biblioteca y en Ajustes";
  - "La repetición se respeta fuera de Reproduciendo";
  - "La cola sigue con la pantalla apagada fuera de Reproduciendo";
  - "Reproduciendo sigue avanzando como antes".
- **Regresión:**
  - L, R, el mini reproductor y el transporte táctil cambian de pista como antes;
  - `playback/track-lifecycle`: con el overlay de debug, el HEAP al volver a la primera pista después de diez cambios automáticos fuera de Reproduciendo es el anotado al empezar, con unos pocos KB de diferencia;
  - `playback/screen-off`: el escenario "Escuchar un álbum entero con la pantalla apagada".

### CI

Sin cambios en el workflow. `tests/Makefile` suma `test_track_advance` a `BIN` y el CI lo corre con los demás.

### Licencias

Código propio. No se agregan bibliotecas ni assets.

### Recursos: quién libera qué

- **La pista no cambia de dueño.** `Music_HandleNext` la desmonta antes de abrir otra, también cuando la llama `Music_Update` desde otra pantalla. Si `Menu_InitMusic` falla a mitad, ya libera lo que tomó (`playback/track-lifecycle`). Si nada abre, no queda nada cargado.
- **`track_advance`** no reserva memoria, no abre archivos ni crea hilos: trabaja sobre las funciones y el contexto del llamador.

## Risks / Trade-offs

- [Un cambio de pista dentro del bucle de escaneo o del diálogo de búsqueda crea texturas en un momento que hoy no ocurre] → Es entre fotogramas, igual que en las pantallas. Se verifica en consola dejando terminar una pista durante un escaneo de la biblioteca y con el teclado de búsqueda abierto.
- [`Utils_ReadControls()` pasa a depender de `menu_audioplayer`] → Es una sola llamada declarada en `menu_audioplayer.h`. `utils.c` ya depende de `audio.h` para el power tick.
- [Al terminar una pista se ve un fotograma más largo en Carpetas o la Biblioteca] → Es el mismo tiempo que hoy tarda en Reproduciendo, que el usuario ya conoce.

## Migration Plan

Sin datos ni formatos nuevos. Se instala encima de v3.5.0. Rollback: reinstalar el `.vpk` de v3.5.0.
