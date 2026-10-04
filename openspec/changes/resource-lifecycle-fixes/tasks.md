## 1. Medir antes de cambiar

- [x] 1.1 En `source/ui_gpu.c`, agregar a `UI_Debug_SampleMemory` la lectura de `mallinfo().uordblks` y una línea "HEAP" en el overlay de debug, con el heap en uso en KB (decisión 6 de design.md; el contador de recogidas diferidas se suma en 2.1). Agrandar `UI_DEBUG_PANEL_H` si hace falta. Listo cuando `scripts/build.sh` compile con `-Wall -Werror` y, en la consola, la cifra cambie al entrar a la Biblioteca o al reproducir. Si `mallinfo` no existe o no se mueve, aplicar la alternativa de Risks y actualizar design.md antes de seguir.
  - Resultado (2026-10-04): `mallinfo` existe en el newlib de VitaSDK y se mueve. En la consola, el HEAP arranca en 1374 KB, sube de 1 a 3 KB al entrar a la Biblioteca y queda entre 1603 y 1612 KB al reproducir.
- [x] 1.2 Verificar en la consola, con el código de 1.1 y sin las correcciones, que las fugas existen: anotar el heap, recorrer tres veces una carpeta de módulos de tracker con R y anotar de nuevo; repetir con la carpeta de archivos dañados. Listo cuando las cifras queden anotadas en esta tarea y el heap haya subido en al menos uno de los dos casos.
  - Resultado (2026-10-04), con los archivos de `ux0:/pruebas-eleven/` generados para la prueba (3 MOD y 3 XM; un WAV bueno y archivos de texto renombrados a `.mp3`, `.ogg`, `.opus`, `.xm` y `.mod`):
    - Tracker, 18 pulsaciones de R (tres vueltas): de 1529 KB a 1536 KB, **+7 KB**.
    - Dañados, 20 pulsaciones de R: de 1529 KB a 1586 KB, **+57 KB**.

## 2. Hilo de audio

- [ ] 2.1 En `source/audio/vitaaudiolib.c`, agregar la generación por hilo, la espera con `sceKernelWaitThreadEnd` y timeout de 1 s, y la lista de pendientes con recogida diferida (decisión 1). En `source/audio/audio.c`, quitar el `sceKernelDelayThread(100 * 1000)` de `Audio_Term`. Exponer el contador de recogidas diferidas y sumarlo a la línea HEAP del overlay. Listo cuando `scripts/build.sh` compile y, en la consola, pasar de pista con R y con el mini reproductor funcione sin cortes ni sonidos superpuestos, en FLAC, MP3, OGG, Opus, WAV y un módulo de tracker.

## 3. Decoders

- [ ] 3.1 En `source/audio/xm.c`, quitar el `strdup` y pasar una copia local de la ruta; liberar el contexto si `xmp_load_module` falla (decisión 2). Listo cuando `scripts/build.sh` compile y, en la consola, recorrer tres veces la carpeta de módulos de tracker deje el heap igual que al principio.
- [ ] 3.2 En `source/audio/mp3.c`, `ogg.c` y `opus.c`, liberar lo tomado en cada camino de error de `*_Init` (decisión 2), y quitar el segundo cierre de `OGG_Term` (decisión 3). Listo cuando `scripts/build.sh` compile y, en la consola, pulsar R veinte veces en la carpeta de archivos dañados deje el heap igual que al principio y la pista buena siga sonando.

- [ ] 3.3 En `source/audio/mp3.c`, pedir a `mpg123_read` los bytes de los canales de la pista y no de estéreo fijo; en `source/audio/opus.c`, pasar a `op_read_stereo` el tamaño del buffer en valores y no en bytes (decisión 7). Listo cuando `scripts/build.sh` compile y, en la consola, la carpeta de todos los formatos, que tiene un MP3 mono, suene completa sin que la aplicación se caiga.

## 4. Nombre de archivo y casos sin memoria

- [ ] 4.1 En `source/menus/menu_audioplayer.c`, copiar el nombre con `"%s"` y comprobar los tres `malloc` de `Menu_InitMusic` (decisión 4). Listo cuando `scripts/build.sh` compile y, en la consola, un WAV sin etiquetas llamado `100% pure %s %d.wav` se muestre con ese nombre exacto en Reproduciendo.
- [ ] 4.2 En `source/library.c`, restaurar el índice anterior si falla la reserva de la pila del escaneo; en `source/dirbrowse.c` y `source/queue.c`, comprobar las reservas del listado y liberar el nodo descartado al listar la raíz (decisión 5). Listo cuando `scripts/build.sh` compile y, en la consola, reescanear la biblioteca, navegar carpetas y reproducir desde una carpeta funcione igual que antes. El caso sin memoria no se puede provocar en la consola; se revisa leyendo el diff.

## 5. Verificación en consola

- [ ] 5.1 Verificar en la consola, con el overlay de debug abierto, los scenarios de `specs/playback/track-lifecycle`:
  1. Carpeta con todos los formatos: anotar el heap, pasar de pista cincuenta veces con R y volver a la primera; el heap queda igual, con unos pocos KB de diferencia como máximo.
  2. Carpeta de módulos de tracker recorrida tres veces: el heap queda igual.
  3. R veinte veces rápido en una carpeta de FLAC: suena una sola pista, y el heap y la memoria libre de usuario quedan igual.
  4. Carpeta con una pista buena y archivos dañados, R veinte veces: la pista buena vuelve a sonar y el heap queda igual.
  5. Elegir diez veces un archivo dañado en la Biblioteca: aparece el aviso cada vez y el heap no cambia.
  6. WAV `100% pure %s %d.wav`: se muestra con su nombre exacto.
  7. MP3 mono de la carpeta de todos los formatos, escuchado completo hasta que pase solo a la siguiente: suena entero y la aplicación no se cae.

  Listo cuando los siete pasos se cumplan y el resultado quede anotado en esta tarea, junto con el contador de recogidas diferidas al final de la prueba.
