## 1. Módulo puro de color de acento (sin cambio de comportamiento)

- [ ] 1.1 Fijar en `tests/test_accent.c` los resultados actuales de `UI_MakeAccentLegible` para `#FF9166`, un violeta oscuro y un azul claro, calculados con el código de hoy antes de moverlo. Crear `source/accent.c` + `include/accent.h` (sin vita2d ni headers SCE) con la conversión HSL, el histograma de tonos, la elección del pico y el ajuste de legibilidad, movidos tal cual desde `ui_theme.c`. `ui_theme.c` queda solo con el muestreo de la textura, que alimenta el histograma. Agregar `source/accent.c` a `add_executable` en `CMakeLists.txt` y `test_accent` a `tests/Makefile`. Listo cuando: el `.vpk` compila con `-Werror`, `make -C tests` pasa (incluidos los valores fijados y una prueba de que un 20 % de color da el mismo pico con 0.06), y en la consola *DAYDREAM* y un MOD muestran el mismo acento que en v3.2.0.

## 2. Umbral de color al 1 %

- [ ] 2.1 Bajar `UI_HUE_MIN_SHARE` de 0.06 a 0.01 y actualizar su comentario. Agregar a `test_accent` estos casos: gris con 4 % de rojo saturado da color con tono rojo; gris con 0.5 % de rojo no da color; blanco, negro y gris uniformes no dan color; gris con ruido de ±6 por canal no da color; con 20 % de color el pico es igual con 0.06 que con 0.01. Listo cuando: compila con `-Werror`, `make -C tests` pasa, y en la consola *Entropy* da un acento rojo, *2* de i-dle uno azul claro, y *DAYDREAM* y *Shoot Me : Youth, Part. 1* el mismo color que en v3.2.0.

## 3. Acento neutro para carátulas sin color

- [ ] 3.1 Reemplazar el booleano de `UI_CoverDominantColor` por la clasificación de tres estados (`ACCENT_COVER_NONE`, `ACCENT_COVER_ACHROMATIC`, `ACCENT_COVER_CHROMATIC`) del design: textura nula o formato no soportado da `NONE`; se pudo muestrear pero no hay color, `ACHROMATIC`. Agregar `UI_ACCENT_NEUTRAL` (`#E6E3EC`) en `ui_theme.h`. Hacer que `UI_Theme_SetAccentFromCoverArt` aplique el neutro con `ACHROMATIC` sin pasarlo por `MakeAccentLegible`, y el fijo con `NONE`. Actualizar los comentarios de `ui_theme.h` sobre el acento. Agregar a `test_accent`: el gris uniforme y la imagen de un canal (escala de grises) dan `ACHROMATIC`; el 4 % de rojo da `CHROMATIC`. Listo cuando: compila con `-Werror`, `make -C tests` pasa, y en la consola *EASY* y *We are i-dle* muestran el acento neutro claro en la barra de progreso y el nav rail, y un MOD o WAV sigue en naranja.

## 4. Color del contenido sobre el acento

- [ ] 4.1 Agregar `ui_color_on_accent` junto a `ui_color_accent` (declaración en `ui_theme.h`, definición en `ui_theme.c`). Fijarlo en `UI_Theme_ApplyAccent`: `UI_COLOR_BG` con el acento neutro y `UI_COLOR_TEXT_PRIMARY` en los demás casos, incluido `UI_Theme_ResetAccent`. Dejar la regla en `accent.c` como función pura con prueba en `test_accent`. Usarlo en los símbolos de play/pausa de `menu_audioplayer.c:302,304` y `mini_player.c:71,73`. Listo cuando: compila con `-Werror`, `make -C tests` pasa, y en la consola, con *EASY*, el símbolo de play y el de pausa se ven oscuros y claros dentro del botón en Now Playing y en el mini player de Folders; con *Entropy* y con un MOD se ven igual que en v3.2.0.

## 5. Verificación en consola y decisión sobre el neutro

- [ ] 5.1 Instalar el `.vpk` en la Vita y recorrer el escenario del acento neutro con *EASY*: Now Playing (play pausado y en reproducción, barra de progreso), Folders (carpeta seleccionada, mini player), Biblioteca (pestaña activa), Settings (categoría activa, interruptor activo) y nav rail. Confirmar que lo activo se distingue de lo inactivo en cada pantalla. Repetir el paso de *EASY* a un MOD (cambia a naranja) y de vuelta. Si hace falta, ajustar el valor de `UI_ACCENT_NEUTRAL`. Listo cuando: todos los escenarios del delta de `ui/dynamic-accent` se ven en la consola como describen. **Si el neutro no se ve bien**, aplicar el criterio de descarte del design: revertir 3.1 y 4.1, ajustar el spec y las notas de versión (patch v3.2.1), y dejar constancia aquí.

## 6. Integración

- [ ] 6.1 Hacer push de `change/neutral-accent-for-achromatic-covers` y abrir el PR a `main`. Listo cuando: el CI compila el `.vpk` en verde en la rama.
