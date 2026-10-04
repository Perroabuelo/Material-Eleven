Rama: `change/resource-lifecycle-fixes`, creada desde `main`. Su PR apunta a `main`.

## Why

La revisión de memoria del 2026-10-03 encontró recursos que se toman al abrir una pista y no siempre se devuelven. Una pista de tracker (MOD/XM/IT/S3M) pierde memoria cada vez que suena. Un archivo que no abre deja tomado su decoder, y como la cola reintenta toda la lista cuando una pista falla, una carpeta con archivos dañados pierde memoria en cada vuelta. Además, el hilo de audio de la pista anterior se da por terminado tras una espera fija de 100 ms. Si tarda más, queda vivo, se suma al de la pista nueva y su memoria no se recupera. Nada de esto se ve en el overlay de debug: el heap se reserva entero al arrancar, así que la memoria libre que muestra no baja aunque se pierda.

## What Changes

- **Cambiar de pista espera de verdad a que termine el hilo de audio anterior**, en vez de dormir 100 ms y suponerlo. Un hilo que no alcanza a terminar no puede volver a sonar ni pisar los buffers de la pista nueva, y se recoge más tarde.
- **Las pistas de tracker dejan de perder memoria** al abrirse, y también cuando no se pueden abrir.
- **Un archivo MP3, OGG u Opus que no abre libera lo que alcanzó a tomar.** Un archivo con extensión `.mp3` que no es MP3 ahora falla al abrirse: la Biblioteca muestra el aviso en vez de llevar a Reproduciendo en silencio.
- **El nombre de un archivo que contiene `%` se muestra tal cual** en Reproduciendo. Hoy se interpreta como formato y puede mostrar basura.
- **Correcciones de robustez sin efecto visible en uso normal:** OGG deja de cerrar dos veces su archivo; el reescaneo de la biblioteca sin memoria ya no pierde el índice; Carpetas y la cola comprueban la reserva del listado antes de leer el directorio; Carpetas deja de perder un nodo cada vez que lista la raíz.
- **Un MP3 mono deja de corromper la memoria y tumbar la aplicación.** El decoder de MP3 suponía estéreo y escribía el doble del buffer de salida. Opus pasaba el tamaño de ese buffer en bytes en vez de en muestras, así que un paquete más largo que el grano de salida podía desbordarlo. Ambos se encontraron al verificar este cambio en la consola, con archivos de prueba mono.
- **El overlay de debug muestra el heap en uso**, para poder comprobar en la consola que cambiar de pista no lo hace crecer.

## Capabilities

### New Capabilities

- `playback/track-lifecycle`: qué se garantiza sobre los recursos de una pista al abrirla, al cambiarla y cuando no se puede abrir.

### Modified Capabilities

Ninguna.

## Impact

- **Código C**:
  - `source/audio/vitaaudiolib.c` y `source/audio/audio.c`: espera y recogida del hilo de audio.
  - `source/audio/xm.c`, `mp3.c`, `ogg.c` y `opus.c`: caminos de error y cierre; en `mp3.c` y `opus.c`, además, el tamaño de lo que se decodifica en cada callback.
  - `source/menus/menu_audioplayer.c`: nombre de archivo sin formato.
  - `source/library.c`, `source/dirbrowse.c` y `source/queue.c`: casos sin memoria.
  - `source/ui_gpu.c`: línea nueva en el overlay de debug.
- **APIs**: ninguna pública cambia de firma.
- **Textos, assets, dependencias y build**: sin cambios.
- **CI**: sin cambios.

## Fuera de alcance

- La pila de pantallas que crece con cada salto. Va en `flat-screen-dispatch`.
- Reducir la carátula de la pista en curso, que hoy se guarda a resolución completa. No crece con el tiempo, pero es el pico de memoria más alto.
- Renovar los atlas de las fuentes TTF cuando se llenan.
- Revisar el código interno de vita2d y de las librerías de decodificación.

## Notas de version

Versión objetivo: **v3.4.2** (patch): corrige fugas de memoria al cambiar de pista.

- Fixed memory leaks when changing tracks, when playing tracker modules (MOD, XM, IT, S3M) and when a file can't be opened.
- Fixed file names containing "%" showing garbled on Now Playing.
- Fixed a crash when playing mono MP3 files, and a possible memory overrun on some Opus files.
