## Context

El porqué está en proposal.md. Acá va el estado del código que condiciona el cómo.

**Por qué el badge no muestra texto.** Las dos llamadas que hay (`dirbrowse.c:303` y `menu_audioplayer.c:404`) invocan `UI_DrawBadge(..., label, bg = wash, fg = color, border = color)`. El wash tiene un alfa de 41 sobre 255. `UI_DrawBadge` hace el borde dibujando primero una píldora llena con `border` y después, 1 px más adentro, otra con `bg`. Ese truco solo sirve si la píldora interior es opaca. Como el wash es casi transparente, el interior queda del color sólido del borde, y el texto, que va en ese mismo color, desaparece. La paleta (`UI_COLOR_LOSSLESS`, `UI_COLOR_LOSSY`, `UI_COLOR_TRACKER` y sus wash) ya coincide con `pic0.png` y con los mockups de `templates/`, así que no cambia.

**Cómo se pierde la señal de mantenerse despierto.** `Utils_LockPower()` hacía dos cosas a la vez: bloqueaba el botón PS (`sceShellUtilLock`) e incrementaba `lock_power`, que habilita al hilo `power_tick_thread` para llamar a `sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND)` cada 10 s. El commit `6070b10` quitó la llamada para que el botón PS volviera a funcionar, y con eso `lock_power` quedó en 0 para siempre: el hilo corre pero nunca hace nada.

**START hoy.** En Carpetas (`menu_displayfiles.c:462`) y en Biblioteca (`menu_library.c:566`) hace `break` del lazo principal, y eso termina saliendo de la app. En Reproduciendo (`menu_audioplayer.c:483`) llama a `scePowerRequestDisplayOff()`. Ajustes y las pantallas de escaneo (`library.c`) no lo tratan. La combinación L + R + START (`FILTER_ABORT_COMBO`) se lee en el lazo que espera la entrada de texto del filtro (`menu_displayfiles.c:194`). Todos los lazos leen el pad a través de `Utils_ReadControls()`, que calcula el flanco `pressed`.

**La barra hoy.** `NavRail_DrawHintBar(y, const char **segments, int count)` dibuja textos sueltos. Por convención, cada pantalla ubica las acciones en posiciones (confirmar, volver, Triángulo, Cuadrado, extras), pero no se cumple en todas. Biblioteca pone "L R - Vistas" en la posición de Triángulo. Reproduciendo anuncia "Menú" en la posición de confirmar, aunque confirmar pausa y reanuda (`menu_audioplayer.c:455`), y no anuncia ni Triángulo (barajar) ni Cuadrado (repetir). La asignación de confirmar y volver se lee una vez al arrancar (`main.c:94`) en `SCE_CTRL_ENTER` y `SCE_CTRL_CANCEL`.

**Filas de Biblioteca.** `Menu_DrawLibraryRow` (`menu_library.c:206`) dibuja tanto filas de canción como de artista o álbum, recortando el texto a `960 - x - 100`. `Library_Track` ya tiene `ext`.

## Goals / Non-Goals

**Goals:**
- Una sola forma de dibujar un badge de formato, sin parámetros que permitan reproducir el defecto.
- Una sola forma de manejar START y el despertar de la pantalla, para todos los lazos, sin repetirla en cada pantalla.
- Que la lógica de qué pulsación se descarta al despertar sea código puro, con prueba en PC.
- Una barra que diga, por construcción, qué botón hace cada cosa.

**Non-Goals:**
- Rediseñar la barra (alto, fondo, posición) o el nav rail.
- Cambiar el comportamiento de cualquier botón distinto de START.
- Detectar por API el estado de la pantalla, si la consola no lo expone de forma simple (ver Riesgos).

## Decisions

### 1. `UI_DrawBadge` pierde el parámetro de borde

La firma pasa a `UI_DrawBadge(x, y, ts, label, bg, fg)`. Se dibuja una sola píldora con `bg` y el texto con `fg` encima. Se suma `UI_BadgeWidth(ts, label)` para que quien lo ubique pueda alinearlo a la derecha.

- **Alternativa descartada:** mantener el borde y dibujarlo como un aro de verdad, con su propio alfa, como en el mockup de `templates/Folders` (borde al 30 %). El usuario eligió el estilo de `pic0.png`, que no tiene borde, y ningún llamador lo necesita. Quitar el parámetro elimina la trampa de raíz.
- **Alineación:** en Carpetas y en Biblioteca, el badge se alinea a la derecha de la fila (`x = borde_derecho - UI_BadgeWidth`), y el texto de la fila se recorta 16 px antes del badge. Así "IT" y "OPUS" terminan en la misma columna. Hoy empiezan en la misma columna y terminan en distintas. En Reproduciendo sigue alineado a la izquierda, bajo el título, como ahora.

### 2. El badge en Biblioteca se pasa como una extensión opcional

`Menu_DrawLibraryRow` recibe un `const char *ext` más. Las filas de canción pasan `track->ext`, y las de artista o álbum pasan `NULL`. Con extensión reconocida (`UI_GetFormatBadge`), la fila dibuja el badge a la derecha y recorta el título y el artista hasta 16 px antes del badge. Sin extensión, todo queda como hoy. Así se cubren solas las cuatro vistas y las pistas dentro de un artista o un álbum, que pasan por la misma rama.

### 3. START y el despertar se manejan en `Utils_ReadControls`

Todos los lazos ya pasan por `Utils_ReadControls()`, así que el comportamiento queda uniforme, incluidas las pantallas de escaneo y Ajustes, que hoy no tratan START.

La decisión de qué hacer con cada flanco se saca a un módulo puro nuevo, `source/screen_off.c` / `include/screen_off.h`, sin vita2d ni SCE:

```
typedef struct { int armed; int enabled; } ScreenOff_State;

// Recibe el flanco y los botones mantenidos de este fotograma, y devuelve
// el flanco que deben ver las pantallas. *action dice si hay que apagar
// la pantalla, encenderla o dejarla como esta.
unsigned int ScreenOff_Filter(ScreenOff_State *s, unsigned int pressed,
                              unsigned int held, ScreenOff_Action *action);
```

Reglas del filtro:
1. Si `enabled` es falso, el flanco pasa sin cambios.
2. Si `armed` es verdadero (la pantalla se apagó con START) y llega cualquier flanco, se pide encender la pantalla, el flanco se descarta entero, `armed` vuelve a falso y se devuelve 0. Comprobado en consola (tarea 2.1): con la pantalla apagada por la aplicación, la consola solo la enciende con el botón PS, pero los demás botones siguen llegando a la aplicación. Sin esta regla, X no encendía la pantalla y además pausaba la reproducción a ciegas en Reproduciendo.
3. Si llega el flanco de START y L y R no están mantenidos a la vez, se pide apagar la pantalla, `armed` pasa a verdadero y START se quita del flanco.
4. En cualquier otro caso, el flanco pasa sin cambios.

`Utils_ReadControls()` aplica el filtro y, según lo que se pida, llama a `scePowerRequestDisplayOff()` o a `scePowerRequestDisplayOn()`. Como `old_pad` se sigue actualizando con el pad real, un botón mantenido no se repite al despertar. Las constantes de botón se pasan desde `utils.c`, y el módulo puro solo recibe máscaras, así que las pruebas en PC no dependen de los headers de la consola.

- **Mientras espera la entrada de texto del filtro**, `menu_displayfiles.c` deshabilita el filtro (`enabled = 0`) y lo vuelve a habilitar al salir. Con eso START no cambia lo que hace mientras el diálogo del sistema está delante, que queda fuera de alcance, y L + R + START sigue siendo solo la cancelación.
- **Se quitan** los `break` de START en Carpetas y en Biblioteca, y la llamada de Reproduciendo, que pasa a hacerla el filtro.
- **Alternativa descartada:** que cada pantalla trate START por su cuenta, como hoy. Repite la lógica en cinco lazos, y el descarte del despertar se olvidaría en alguno.

### 4. Mantenerse despierto separado del botón PS

El hilo `power_tick_thread` deja de mirar `lock_power` y llama a `sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND)` cuando `Audio_HasTrack() && !Audio_IsPaused()`. Las dos funciones devuelven un flag simple (`track_loaded`, `paused`), así que leerlas desde otro hilo es tan seguro como cualquier lectura de un entero alineado. Si el valor se lee desfasado, el efecto dura como mucho un ciclo de 10 s, muy por debajo del temporizador más corto del sistema. Se borran `Utils_LockPower`, `Utils_UnlockPower` y `lock_power`, que nadie llama. Así no queda ninguna API cuyo nombre sugiera que mantener despierto y bloquear el PS van juntos.

- **Alternativa descartada:** devolver `Utils_LockPower()` a la reproducción. Vuelve a bloquear el botón PS, que es justo el defecto que corrigió `6070b10`.

### 5. La barra recibe entradas de botón y texto

```
typedef enum {
    HINT_BTN_CONFIRM, HINT_BTN_CANCEL,      // se resuelven al dibujar con SCE_CTRL_ENTER/CANCEL
    HINT_BTN_TRIANGLE, HINT_BTN_SQUARE,
    HINT_BTN_L, HINT_BTN_R, HINT_BTN_SELECT, HINT_BTN_START,
    HINT_BTN_NONE
} NavRail_HintButton;

typedef struct {
    NavRail_HintButton buttons[3];  // hasta tres; HINT_BTN_NONE termina la lista
    int combo;                      // 1: se dibuja "+" entre botones (a la vez); 0: van juntos (cualquiera)
    const char *label;              // NULL: la entrada no se dibuja
} NavRail_Hint;

void NavRail_DrawHints(float y, const NavRail_Hint *hints, int count);
```

Lleva otro nombre que la función de textos para poder convivir con ella mientras se migran las llamadas. La de textos se borra al final de la migración.

- **Íconos.** Cruz, Círculo, Cuadrado y Triángulo se dibujan como geometría con las primitivas de `ui_theme` (`UI_DrawStroke`, `vita2d_draw_fill_circle`, `UI_DrawTriangle`), en contorno, dentro de una caja de unos 18 px centrada en la línea base del texto. Cumple `ui/rendering`: ni texturas ni glifos de fuente, que además no garantizan esos símbolos.
- **Colores.** Se agregan al tema cuatro tokens: `UI_COLOR_BTN_CROSS` (azul), `UI_COLOR_BTN_CIRCLE` (rojo), `UI_COLOR_BTN_SQUARE` (rosa) y `UI_COLOR_BTN_TRIANGLE` (verde). Se toman los tonos clásicos de los símbolos de PlayStation y se aclaran hasta que contrastan con `UI_COLOR_BG_ELEVATED` (el fondo de la barra) lo mismo que `UI_COLOR_TEXT_SECONDARY`, con el mismo cálculo de contraste que ya usa el acento dinámico (`UI_ContrastOverBg`). Los valores finales quedan en `ui_theme.h`.
- **Chips neutros.** L, R, SELECT y START se dibujan con el nombre en `UI_FACE_MONO` sobre una píldora `UI_COLOR_SURFACE_2`, en `UI_COLOR_TEXT_SECONDARY`, sin borde, igual que el badge.
- **Confirmar y volver** se resuelven en cada dibujo comparando `SCE_CTRL_ENTER` con `SCE_CTRL_CROSS`. Así la barra no puede mostrar un botón distinto del que la pantalla escucha.
- **Si no entra todo**, las entradas que no caben en el ancho disponible no se dibujan. Por eso cada pantalla ordena sus entradas de más a menos importante.
- **START en la barra.** Cada pantalla agrega al final la entrada "[START] Apagar pantalla" / "Screen off", que es la de menor prioridad. Si no cabe, no se dibuja y no se solapa con nada.

**Entradas por pantalla**, revisadas contra lo que cada lazo escucha:

| Pantalla | Entradas |
|---|---|
| Carpetas | confirmar Abrir / Reproducir (si hay entradas), volver Carpeta superior o Quitar filtro (según el estado), SELECT Ajustes, START Apagar pantalla |
| Elegir carpeta (Ajustes, almacenamiento) | confirmar Entrar, volver Carpeta superior o Cancelar, Triángulo Elegir esta carpeta |
| Biblioteca | confirmar Reproducir, Abrir o Empezar, volver Volver (si se está adentro), L·R Vistas, Triángulo Reescanear, Cuadrado Carpeta, START Apagar pantalla |
| Biblioteca escaneando (`library.c`) | volver Abandonar |
| Reproduciendo | confirmar Pausa o Reproducir (lo que haga ahora), volver Atrás, L·R Anterior / Siguiente, Triángulo Aleatorio, Cuadrado Repetir, START Apagar pantalla |
| Ajustes | confirmar Seleccionar, volver Atrás, L·R Cambiar de categoría |
| Entrada de texto del filtro | L+R+START Cancelar la búsqueda (`combo = 1`) |

**Ancho medido.** Con la fuente mono de 17 px (unos 10 px por carácter) quedan unos 840 px útiles. "Reproducir / Pausa" no cabe en Reproduciendo en español, ni siquiera sin START. Por eso confirmar anuncia lo que hará ahora: "Pausa" mientras suena y "Reproducir" en pausa, la misma convención que ya sigue Carpetas con "Quitar filtro". `STR_HINT_PLAY_PAUSE` se reemplaza por `STR_HINT_PAUSE` y se reutiliza `STR_HINT_PLAY`. La separación entre entradas baja de 26 a 22 px. Según la estimación, "Apagar pantalla" se cae en español en Reproduciendo y dentro de un artista o un álbum de Biblioteca, que es justo el caso previsto. En Carpetas cabe por poco.

Esto corrige de paso la barra de Reproduciendo, que anunciaba "Menú" en confirmar y no mostraba barajar ni repetir. Se agregan `STR_HINT_SHUFFLE`, `STR_HINT_REPEAT` y `STR_HINT_SCREEN_OFF`, y se quita `STR_HINT_MENU` si queda sin uso. Los textos que hoy nombran el botón (`STR_HINT_SETTINGS_CATEGORY`, `STR_HINT_CANCEL_SEARCH`, `STR_HINT_CHOOSE_FOLDER`, `STR_HINT_SETTINGS`, `STR_HINT_VIEWS`, `STR_HINT_RESCAN`, `STR_HINT_FOLDER`, `STR_HINT_PREV_NEXT`) pierden el prefijo en los dos idiomas, y `STR_HINT_EXIT` se borra.

- **Alternativa descartada:** dejar la API de textos y dibujar el ícono según la posición en el arreglo. Es la convención que ya no se cumple en Biblioteca ni en Reproduciendo. Una estructura explícita hace que cada entrada diga su botón.

## Risks / Trade-offs

- **[Botones a ciegas con la pantalla apagada]** → Resuelto con la regla 2. La prueba de la tarea 2.1 mostró que los botones llegan a la app con la pantalla apagada, y que la consola solo la enciende con PS. La app la enciende con la primera pulsación y la descarta.
- **[Encender la pantalla con el botón PS]** → La app no ve el botón PS, así que el filtro sigue armado y descarta la primera pulsación posterior, que ya no hace nada porque la pantalla está encendida. Es un costo menor, una pulsación que hay que repetir, y es mejor que abrir una fila sin querer. Si en consola resulta molesto, se puede investigar si el callback de energía `SCE_POWER_CB_UNK_0x100000` (asociado al cambio de pantalla y permitido para apps comunes) avisa del encendido.
- **[Contraste de los colores de PlayStation]** → El rojo y el azul originales son oscuros sobre `#17141F`. Se aclaran con el mismo cálculo que el acento, y se verifica en consola que se lean y que se distingan entre sí.
- **[Ancho de la barra en español]** → Los textos en español son más largos. Las entradas que no caben no se dibujan, y la tarea de la barra verifica en consola, en los dos idiomas, que en ninguna pantalla se caiga una entrada distinta de START.
- **[Quien usaba START para salir]** → La nota de versión lo dice, y el botón PS cierra la app como en cualquier otra aplicación.
- **[Batería con la pantalla apagada]** → Mientras suena un track la consola ya no se suspende, que es justo lo que se busca. En pausa vuelve a suspenderse.

## Migration Plan

No hay datos ni configuración que migrar. Se instala encima de la v3.1.1 y conserva la biblioteca y los ajustes. Para volver atrás basta con instalar el `.vpk` anterior.

## Pruebas

- **Compilación**: el `.vpk` compila limpio con `-Wall -Werror` después de cada tarea.
- **Tests en PC**:
  - `tests/test_screen_off.c`, nuevo en `tests/Makefile`, cubre las reglas del filtro: START apaga y se quita del flanco, L + R + START pasa sin apagar, deshabilitado no toca nada, la pulsación siguiente al apagado se descarta y la de después pasa.
  - `test_lang`, que ya existe, verifica que los textos nuevos y modificados estén en los dos idiomas.
- **Consola**: cada escenario de los specs, paso a paso, en las tareas. Sobre todo:
  - una cola más larga que el temporizador de ahorro de energía, con la pantalla apagada;
  - la barra con la consola en confirmar con Cruz y en confirmar con Círculo;
  - los nueve formatos en Carpetas y en Biblioteca.

## CI

Sin cambios. El workflow compila el `.vpk` y no corre los tests en PC. Correrlos en CI es una mejora aparte.

## Licencias

No se incorpora código ni assets de terceros. Los íconos de los botones son geometría propia. Los colores de los símbolos de PlayStation se usan solo como referencia de tono para identificar los botones físicos de la consola.
