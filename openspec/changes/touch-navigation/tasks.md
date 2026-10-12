## 1. Preparación

- [x] 1.1 Crear la rama `change/touch-navigation` desde `main` con `key-repeat` y `library-search` ya integrados, y comprobar que el `.vpk` compila y `make -C tests` pasa antes de tocar nada

## 2. Lógica pura

- [ ] 2.1 Agregar `gesture.c`/`gesture.h` (apoyo, toque, arrastre con umbral, salida con velocidad promediada, gesto "atrapado") con `tests/test_gesture.c` en `tests/Makefile`; listo cuando `make -C tests` pasa con ASan/UBSan y cubre toque, arrastre, levantar fuera del área y ruido de un fotograma
- [ ] 2.2 Agregar `list_scroll.c`/`list_scroll.h` (arrastre acotado, inercia por `dt_ms`, detener, `EnsureVisible`, `RowAt`) con `tests/test_list_scroll.c`; listo cuando `make -C tests` pasa y cubre extremos, lista corta, inercia monótona y finita, `dt_ms` grande, fila a medias y lista vacía
- [ ] 2.3 Sumar los dos fuentes a `CMakeLists.txt` sin usarlos todavía; listo cuando el `.vpk` compila con `-Werror`

## 3. Base de dibujo y entrada

- [ ] 3.1 Reemplazar el `malloc` de `touchState` en `touch.c` por estado estático y exponer el flanco de apoyo y la posición que necesita `gesture`; listo cuando compila y en consola el rail, el mini reproductor y el transporte de Reproduciendo responden igual que antes
- [ ] 3.2 Agregar `UI_PushClip`/`UI_PopClip` en `ui_theme.c` y migrar `UI_DrawTextClipped` a la pila; listo cuando compila y en consola los textos recortados de las cuatro pantallas se ven igual en English y Español

## 4. Biblioteca

- [ ] 4.1 Pasar el dibujo de la lista de la Biblioteca a `list_scroll` con recorte al área y `EnsureVisible` tras botones, cambio de vista, entrar/salir de un grupo y volver a la pantalla; listo cuando en consola los botones (incluida la repetición) y volver de Reproduciendo a la misma fila funcionan como antes
- [ ] 4.2 Conectar `gesture` a la lista de la Biblioteca según el orden de dueños del toque: arrastre, inercia, resaltado al apoyar y activación al soltar; listo cuando en consola pasan los escenarios de desplazamiento, inercia, toque y "cada toque lo recibe un solo elemento" sobre la Biblioteca
- [ ] 4.3 Hacer tocables las pestañas, las acciones de los estados vacíos, la caja de búsqueda y las entradas del menú de □; listo cuando en consola cada uno hace lo mismo que su botón

## 5. Carpetas

- [ ] 5.1 Pasar `Dirbrowse_DisplayFiles` y la lista de elegir carpeta a `list_scroll` con recorte y `EnsureVisible` tras botones, entrar/salir de carpeta y aplicar o quitar el filtro; listo cuando en consola la navegación por botones y el filtro se comportan como antes
- [ ] 5.2 Conectar `gesture` a la lista de Carpetas y a la de elegir carpeta de la biblioteca (arrastre, inercia, resaltado, abrir carpeta o archivo al soltar, también `..`); listo cuando en consola pasan los escenarios de spec sobre Carpetas, incluido el mini reproductor con una fila detrás

## 6. Ajustes

- [ ] 6.1 Hacer tocables las categorías (selecciona y pone la primera opción) y las opciones (mismo efecto que Confirmar), con `list_scroll` en la lista de opciones; listo cuando en consola pasan "Tocar una categoría de Ajustes" y "Tocar una opción de Ajustes" y el cambio persiste tras reiniciar

## 7. Verificación y cierre

- [ ] 7.1 Verificar en consola todos los escenarios de los requisitos que este cambio agrega en `specs/ui/list-navigation/spec.md` paso a paso y, con el overlay de debug, anotar pila y HEAP en Carpetas antes y después de recorrer veinte veces Biblioteca (arrastre con inercia, tocar un álbum, volver) → Carpetas (arrastre) → Ajustes; listo cuando todo pasa y pila y HEAP no crecen, con las constantes de gesto e inercia ya afinadas
- [ ] 7.2 Agregar las notas de v3.9.0 a `CHANGELOG.md` desde la proposal y abrir el PR a `main`; listo cuando el CI está en verde
