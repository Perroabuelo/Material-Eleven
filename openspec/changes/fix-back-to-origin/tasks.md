## 1. Volver al origen

- [x] 1.1 En `source/menus/menu_audioplayer.c`, agregar `static UI_Screen playback_origin = UI_SCREEN_FOLDERS`. Ponerla en `UI_SCREEN_FOLDERS` en `Menu_PlayAudio` y en `UI_SCREEN_LIBRARY` en `Menu_PlayQueued`, en los dos casos después de que `Menu_InitMusic` haya abierto la pista. Cambiar la rama `SCE_CTRL_CANCEL` de `Menu_RunNowPlayingLoop` para que llame a `Menu_DisplayLibrary()` cuando el origen sea la Biblioteca y a `Menu_DisplayFiles()` en otro caso (decisión 1 de design.md). Listo cuando `scripts/build.sh` compile con `-Wall -Werror`.

## 2. Verificación en consola

- [ ] 2.1 Verificar en consola, primero con la consola configurada para confirmar con Cruz y después con Círculo, los scenarios de `specs/ui/nav-shell`:
  1. Desde Carpetas, entrar a una subcarpeta, reproducir el tercer archivo y pulsar volver: se vuelve a esa carpeta con el tercer archivo seleccionado.
  2. Desde la Biblioteca, en Álbumes, abrir un álbum, reproducir la tercera pista y pulsar volver: se vuelve a Álbumes, dentro del álbum, con la tercera pista seleccionada.
  3. Repetir el paso 2 desde Artistas, Canciones y Añadidos recientemente.
  4. Reproducir desde la Biblioteca, ir a Ajustes con el nav rail, volver a Reproduciendo con el nav rail y pulsar volver: se vuelve a la Biblioteca.
  5. Reproducir desde la Biblioteca, pulsar R para pasar a la pista siguiente y pulsar volver: se vuelve a la Biblioteca.
  6. Reproducir desde la Biblioteca, ir a Carpetas con el nav rail, reproducir un archivo ahí y pulsar volver: se vuelve a Carpetas.
  7. SELECT en Carpetas sigue abriendo Ajustes, y START sigue apagando la pantalla.

  Listo cuando los siete pasos se cumplan y el resultado quede anotado en esta tarea.
