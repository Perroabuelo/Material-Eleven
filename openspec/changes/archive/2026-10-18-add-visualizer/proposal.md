## Why

Reproduciendo muestra la carátula, la letra y el transporte, pero nada en la pantalla reacciona a la música que suena. El visualizador de audio quedó fuera de alcance en `add-playlists` y es lo que falta para que Reproduciendo se sienta vivo mientras la consola está en la mano.

## What Changes

- **Vista de visualizador en Reproduciendo.** D-pad abajo alterna entre la vista que estaba abierta y una vista de visualizador a pantalla completa, dentro de Reproduciendo, con el nav rail y la leyenda de botones. Igual que la vista de letras, muestra una cabecera con la pista, la zona del visualizador en el centro y la barra de progreso abajo.
- **Salto directo entre letras y visualizador.** Desde la vista de letras, D-pad abajo abre el visualizador; desde el visualizador, D-pad arriba abre las letras. Pulsar de nuevo el botón de la vista abierta vuelve a la vista normal.
- **Cinco estilos que se recorren con ← y →** (y deslizando el dedo a los lados sobre la zona del visualizador), dando la vuelta al llegar al final, con un indicador de puntos que dice cuál está activo:
  - **Espectro:** barras de frecuencias, de graves a agudos.
  - **Osciloscopio:** la forma de onda de lo que suena.
  - **VU:** medidores de nivel de los canales izquierdo y derecho, con indicador de pico.
  - **Disco + espectro radial:** la carátula como etiqueta de un vinilo que gira, con barras de frecuencias alrededor del borde.
  - **Disco que respira:** el mismo vinilo, que late con los graves y tiene un halo del color de acento de la pista.
- **El vinilo.** La carátula se recorta en círculo y hace de etiqueta central, rodeada de surcos dibujados como geometría vectorial y con el borde suavizado. Gira mientras la música suena y, al pausar, frena hasta detenerse en vez de pararse en seco. Una pista sin carátula muestra un vinilo con una etiqueta lisa.
- **Sincronía.** Lo que se dibuja corresponde a lo que se está oyendo, no a lo que se acaba de decodificar.
- **Pausa y silencio.** Al pausar, las barras y los medidores caen a cero de forma continua.
- **Memoria de sesión.** La vista y el estilo elegidos se mantienen al cambiar de pista y al salir y volver a Reproduciendo, como la vista de letras. La app arranca siempre en la vista normal y con el estilo Espectro.
- **Leyenda.** Reproduciendo anuncia D-pad abajo ("Visualizador") en todas sus vistas, y ← → ("Estilo") en la vista de visualizador.
- **Log de desarrollo.** La línea `nowplaying` gana los campos `view` y `style`, para verificar en la consola la vista y el estilo abiertos.

## Capabilities

### New Capabilities

- `ui/visualizer`: la vista de visualizador de Reproduciendo, cómo se abre, se cierra y se recorre, qué muestra cada uno de los cinco estilos, el vinilo que gira, la sincronía con lo que suena, el comportamiento en pausa, en silencio, con pistas mono y con la pantalla apagada, y la memoria de sesión de la vista y el estilo.

### Modified Capabilities

- `ui/now-playing`: la vista de letras ahora convive con la de visualizador (D-pad arriba desde el visualizador abre las letras), y la leyenda de Reproduciendo anuncia también el visualizador y el cambio de estilo.
- `ui/nav-shell`: la leyenda de botones admite D-pad abajo y el par ← → como chips neutros con flechas dibujadas, igual que el D-pad arriba.
- `dev/debug-log`: la línea `nowplaying` agrega al final los campos `view` y `style`.

## Impact

- **Audio:** una toma de las muestras ya decodificadas en `source/audio/audio.c` (`Audio_Decode`), escrita desde el hilo de audio a un buffer circular que lee la UI. No cambia lo que suena.
- **Código nuevo, puro y probado en PC:** buffer circular de muestras, FFT, agrupado en bandas, suavizado con caída de picos, nivel RMS y de pico por canal, energía de graves, giro del vinilo con inercia y geometría del disco (UV del abanico). Tests nuevos en `tests/`.
- **UI:** `source/menus/menu_audioplayer.c` (vista nueva, toggles, estilos y gesto), un módulo nuevo de dibujo del visualizador, `source/nav_rail.c` e `include/nav_rail.h` (chips de D-pad abajo y de ← →), `include/lang_strings.h` (textos en inglés y español).
- **Dibujo:** la carátula como disco usa `vita2d_draw_array_textured`, ya incluido en el vita2d de VitaSDK.
- **Log:** `source/menus/menu_audioplayer.c` (campos `view` y `style` de `nowplaying`).
- **Documentación:** README (cómo abrir el visualizador y cambiar de estilo) y CHANGELOG.
- Sin dependencias nuevas ni cambios de configuración en disco.

## Rama

`change/add-visualizer`

## Fuera de alcance

- **Recordar el estilo entre arranques** o elegirlo desde Ajustes. El estilo dura la sesión, igual que la vista de letras.
- **Convertir la carátula de la vista normal en disco.** La vista normal queda como está, con la carátula cuadrada.
- **Reflejar el EQ y el ALC de la consola** en el visualizador. La consola los aplica después del punto donde se toman las muestras.
- **Personalizar los estilos** (cantidad de barras, colores, sensibilidad, velocidad de giro).
- **Visualizador en el mini reproductor** o en otras pantallas.
- **Visualizador a pantalla completa sin cabecera ni nav rail**, o con la pantalla apagada.
- **Estilos adicionales**, como partículas o espectrograma.

## Notas de version

Versión objetivo: **v4.4.0** (minor: funcionalidad nueva). En la consola se ve como `04.40`.

- Now Playing has a visualizer: press Down on the D-pad to open it, and again to close it.
- Press Left or Right, or swipe sideways, to switch between five styles: spectrum bars, oscilloscope, VU meters, a spinning vinyl with a radial spectrum, and a spinning vinyl that pulses with the bass.
- The vinyl uses the album art as its label and slows to a stop when you pause.
- Press Up in the visualizer to jump to the lyrics, and Down in the lyrics to jump back.
- The visualizer and the chosen style stay open when the track changes or you leave Now Playing and come back.
