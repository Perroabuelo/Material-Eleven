## Why

Algunas carátulas con color, y todas las que son blanco y negro, no cambian el acento: la interfaz se queda con el naranja fijo, igual que con una canción sin carátula. En la biblioteca de prueba (152 álbumes en la consola) pasa con 16, entre ellas *The Book of Us : Entropy* (DAY6), *2* (i-dle) y *EASY* (LE SSERAFIM, el álbum de *Smart*). El usuario siente que la app trata esas portadas como si no tuvieran carátula.

La causa no es un error de lectura. Es una regla del extractor que no está en el spec: si menos del 6 % de los píxeles muestreados tiene color, la carátula se trata como si no existiera. Esa regla junta dos casos distintos:

- **Portadas con un detalle de color pequeño.** Entropy (texto rojo, 4.1 %), *2* (cromado azulado, 5.2 %) y la tribute SSaW (marco dorado, 3.5 %) tienen un color reconocible que queda apenas bajo el corte.
- **Portadas realmente sin color.** EASY (papel crema con relieve, croma promedio 0.008), las fotos en blanco y negro de DPR IAN, *We are i-dle*, Jason Mraz, Sunhead, etc. No hay color que extraer. Pero tampoco corresponde decir "no hay carátula".

## What Changes

- **Umbral de color más bajo.** El mínimo de píxeles con color para que una carátula aporte su color baja de 6 % a 1 %. Las 136 portadas que hoy ya aportan color no cambian: el umbral solo decide si se usa el color fijo, no qué color se elige. Entropy, *2* y SSaW recuperan su color.
- **Acento neutro para carátulas sin color.** Una carátula que existe pero no tiene color usable da un acento neutro claro, casi blanco, en vez del naranja fijo. El naranja queda solo para "no hay carátula" (WAV, trackers, archivos sin imagen embebida) y "no hay track cargado".
- **Color de contenido sobre el acento.** Los símbolos de play/pausa se dibujan sobre un relleno de color de acento, en Now Playing y en el mini player. Hoy usan siempre el texto primario (casi blanco), que sobre el neutro claro no se vería (contraste 1.1:1). Esos símbolos pasan a usar un color que acompaña al acento: el texto primario con acentos de color y el fondo oscuro con el acento neutro.

## Capabilities

### New Capabilities

Ninguna.

### Modified Capabilities

- `ui/dynamic-accent`: el acento distingue entre "sin carátula" (naranja fijo) y "carátula sin color" (neutro). Las carátulas con un detalle de color pequeño aportan ese color. El contenido dibujado sobre un relleno de acento sigue legible con cualquier acento.

## Impact

- `source/ui_theme.c`: la constante `UI_HUE_MIN_SHARE`, el resultado de `UI_CoverDominantColor` (pasa a distinguir "sin carátula" de "sin color"), `UI_Theme_SetAccentFromCoverArt` y un nuevo color global para el contenido sobre el acento, junto a `ui_color_accent`.
- `include/ui_theme.h`: la declaración del color nuevo y la constante del acento neutro.
- `source/menus/menu_audioplayer.c` y `source/mini_player.c`: el símbolo de play/pausa usa el color nuevo.
- Sin cambios en dependencias, licencias ni CI.

## Rama

`change/neutral-accent-for-achromatic-covers`

## Fuera de alcance

- **Recuperar tintes muy leves** (bajar la saturación mínima de un píxel). Se evaluó: recupera dos portadas más (*The Weekend*, *Nothing Lasts Forever*), pero mueve levemente el color de 85 portadas que hoy funcionan y exagera esos tintes, porque el ajuste de legibilidad lleva la saturación a 0.60 como mínimo.
- **Detectar detalles de color por debajo del 1 %**, como la flecha roja de *The Demon* (0.4 %) o el logo de *ARCHIVES* (0.1 %). Esos casos quedan con el acento neutro.
- **Cambiar el naranja fijo**, el fondo o los colores de texto.
- **Cambiar la extracción** fuera del umbral: muestreo, histograma de tonos y ajuste de legibilidad quedan igual.

## Notas de version

Versión objetivo: **v3.3.0** (minor: un comportamiento visible nuevo, el acento neutro, más una corrección).

- Las carátulas con un detalle de color pequeño, como un título rojo sobre fondo gris, ahora tiñen la interfaz con ese color.
- Las carátulas en blanco y negro le dan a la interfaz un acento neutro claro, en lugar del naranja que se usa cuando una canción no tiene carátula.
