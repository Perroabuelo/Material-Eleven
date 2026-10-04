## 1. Medir antes de cambiar

- [ ] 1.1 En `source/ui_gpu.c` e `include/ui_gpu.h`, agregar `UI_Debug_MarkStackBase()` y una línea "pila" en el overlay de debug con la pila usada y el máximo de la sesión (decisión 4 de design.md). Llamarla al principio de `main()`. Agrandar `UI_DEBUG_PANEL_H` si hace falta para que la línea entre. Listo cuando `scripts/build.sh` compile con `-Wall -Werror` y, en la consola, el overlay muestre la línea nueva.
- [ ] 1.2 Verificar en la consola, con el código de 1.1 y sin el despachador, que la pila crece: anotar la cifra en Carpetas, recorrer Biblioteca → Ajustes → Reproduciendo → Carpetas con el nav rail cinco veces y anotar la cifra otra vez. Listo cuando las dos cifras queden anotadas en esta tarea y la segunda sea mayor que la primera.

## 2. Pedido diferido de pantalla

- [ ] 2.1 Mover el `enum UI_Screen` de `include/ui_theme.h` a `include/ui_screen.h`, sin dependencias, e incluirlo desde `ui_theme.h`. Listo cuando `scripts/build.sh` compile sin tocar ningún otro `#include`.
- [ ] 2.2 Agregar `include/nav_request.h` y `source/nav_request.c` con `NavRequest_Set` y `NavRequest_Take` (decisión 2), sin vita2d ni SCE. Agregar `tests/test_nav_request.c` (sin pedido devuelve `UI_SCREEN_NONE`; `Take` devuelve el pedido y lo borra; gana el último pedido) y sumarlo a `tests/Makefile`. Listo cuando `make -C tests` pase completo y `scripts/build.sh` compile.

## 3. Despachador

- [ ] 3.1 Convertir las cuatro pantallas y `main.c` en un solo paso, porque las firmas cambian juntas. `Menu_DisplayFiles`, `Menu_DisplayLibrary`, `Menu_DisplaySettings` y `Menu_ShowNowPlaying` devuelven `UI_Screen`, y cada `Menu_X(); return;` pasa a `return UI_SCREEN_X;`. Reproduciendo devuelve `playback_origin` al volver y cuando `track_failed` está puesto (decisión 3). `Menu_PlayAudio` y `Menu_PlayQueued` dejan de entrar al lazo de Reproduciendo y llaman a `NavRequest_Set(UI_SCREEN_NOW_PLAYING)`. Carpetas y Biblioteca revisan `NavRequest_Take()` después de sus controles. `main.c` corre el lazo despachador con `Touch_Reset()` y `NavRequest_Take()` en cada cambio (decisión 1). Listo cuando `scripts/build.sh` compile y `grep -n "Menu_Display\|Menu_ShowNowPlaying" source/menus source/dirbrowse.c` no muestre ninguna llamada, solo definiciones.

## 4. Listado de Carpetas sin recursión

- [ ] 4.1 En `source/dirbrowse.c`, reemplazar `Dirbrowse_RecursiveFree` por un lazo que libere nodo por nodo (decisión 5). Listo cuando `scripts/build.sh` compile y, en la consola, entrar y salir diez veces de una carpeta con muchos archivos funcione igual que antes.

## 5. Verificación en consola

- [ ] 5.1 Verificar en la consola, con el overlay de debug abierto, los scenarios de `specs/ui/nav-shell`:
  1. Anotar la pila en Carpetas, recorrer con el nav rail Biblioteca → Ajustes → Reproduciendo → Carpetas diez veces: la pila en Carpetas es la misma que la anotada.
  2. Reproducir desde Carpetas, volver, reproducir desde la Biblioteca, volver, diez veces: la pila en Carpetas y en la Biblioteca no cambia.
  3. SELECT en Carpetas y volver desde Ajustes, veinte veces: la pila en Carpetas no cambia.
  4. Alternar Carpetas y Biblioteca con el nav rail durante cinco minutos, reproduciendo de vez en cuando: la app sigue abierta.
  5. Repetir los siete pasos de verificación de `fix-back-to-origin` (tarea 2.1 de su archivo): la vuelta al origen se comporta igual.
  6. Reproducir desde Carpetas y desde la Biblioteca con el mini reproductor a la vista y usar sus controles: funcionan igual.

  Listo cuando los seis pasos se cumplan y el resultado quede anotado en esta tarea.
