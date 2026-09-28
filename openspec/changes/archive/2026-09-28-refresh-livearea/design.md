## Context

La motivación está en `proposal.md`, sección Why. Estado actual:

- **Las imágenes del sistema** están en `sce_sys/` y se empaquetan desde `vita_create_vpk` (`CMakeLists.txt:123-132`) con líneas `FILE` explícitas: `icon0.png` (128x128), `livearea/contents/bg.png` (840x500), `livearea/contents/startup.png` (280x158) y `template.xml` (estilo `a1`, que referencia `bg.png` y `startup.png`). Las tres imágenes son PNG de paleta (color type 3), de 8 bits. No hay `pic0.png`.
- **El nombre** sale de `VITA_APP_NAME "Eleven Music Player"` (`CMakeLists.txt:14`), que `vita_create_vpk` escribe en el `param.sfo`. El Title ID es `ELEVENMPV`.
- **El diseño está en `docs/assets/LiveArea Assets.dc.html`**, sin versionar, junto con `support.js` y `.thumbnail`. Es HTML y CSS plano, con un SVG inline para la nota. Cada imagen es un `div` con un id (`exp-icon0-a`, `exp-startup`, `exp-bg`, `exp-pic0`) y el tamaño exacto de su PNG. Las fuentes se cargan desde `fonts/Manrope.ttf` y `fonts/IBMPlexMono-Medium.ttf`, rutas relativas que no existen en `docs/assets`. Esos archivos están en `res/`.
- **`support.js` no se necesita.** Es el runtime de la herramienta de diseño, y se verificó con Edge headless que la página renderiza igual sin él.
- **Los colores del diseño son tokens de `include/ui_theme.h`**: `UI_COLOR_BG`, `BG_ELEVATED`, `SURFACE`, `SURFACE_2`, `UI_ACCENT_FIXED`, los de texto y los de lossless, lossy y tracker.
- **Herramientas disponibles**: en esta máquina están Edge (Windows) y `python3` (Windows y WSL). No están `pngquant` ni Pillow.

## Goals / Non-Goals

**Goals:**

- Que las cuatro imágenes que lleva el `.vpk` cumplan siempre el formato que exige la consola, y que eso se verifique antes de instalar.
- Que el `.vpk` y el CI no dependan de las herramientas de generación.
- Que el repositorio no cargue con una fuente del arte ni con herramientas para un diseño que se hace una vez.

**Non-Goals:**

- Un pipeline de assets, ni para estas imágenes ni para las de dentro de la app, que hoy son vectoriales.
- Generar el arte en CI.
- Poder regenerar el arte desde el repositorio. Si cambia, se vuelve a exportar y se reemplazan los PNG.

## Decisions

### 1. Los PNG son los assets, y el diseño no se versiona

Los PNG de `sce_sys/` son lo único que queda en el repositorio, igual que hoy con el arte de ElevenMPV. El HTML de `docs/assets`, `support.js` y `.thumbnail` se usan una vez para generarlos y después se borran.

Por qué: el arte de la LiveArea se diseña una vez y casi nunca cambia. Versionar el HTML suma un segundo lugar donde vive el diseño, que puede desalinearse de los PNG, y obliga a mantener un script y sus dependencias para una tarea que se hace de vez en cuando.

*Alternativa descartada:* versionar el HTML como fuente, junto con un script que genere los PNG. Era el plan anterior. Suma mantenimiento sin un beneficio real para cuatro imágenes que casi nunca cambian.

*Alternativa descartada:* generar los PNG al compilar. Metería un navegador y `pngquant` en el build y en la imagen de CI.

### 2. Generación de una sola vez

Para cada imagen:

1. Se extrae del HTML el `div` con su id, junto con el bloque `<style>` de fuentes. Las rutas de fuentes se apuntan a `res/`.
2. Se escribe una página temporal con `body { margin: 0 }` y ese elemento en (0, 0).
3. Se captura con un navegador headless (Edge o Chromium), con `--window-size` igual al tamaño de la imagen, `--force-device-scale-factor=1` y `--hide-scrollbars`.
4. Se cuantiza el resultado con `pngquant 256 --nofs --strip`.
5. Se escribe el PNG en su lugar de `sce_sys/`.

Las herramientas y la página temporal viven fuera del repositorio. No queda ningún script versionado.

`--nofs` desactiva el dithering. Son colores planos, y el dithering metería ruido visible en los fondos lisos. Cuantizar en Python puro queda descartado: el antialiasing de bordes y texto puede pasar de 256 colores, y `pngquant` ya resuelve el problema con buena calidad.

### 3. Chequeo de formato en la verificación

Antes de versionar los PNG se lee el IHDR de las cuatro imágenes y se exige:

- Firma PNG.
- Color type 3 y bit depth 8.
- El tamaño exacto de cada una.

Es un chequeo de la tarea, sin script en el repositorio. Se hace con cualquier herramienta que lea el encabezado PNG, y se confirma que detecta un error probándolo con un PNG RGBA de prueba.

### 4. Nombre y splash en `CMakeLists.txt`

- `VITA_APP_NAME` pasa a `"Material Eleven"`.
- `vita_create_vpk` suma `FILE sce_sys/pic0.png sce_sys/pic0.png`.
- `VITA_VERSION` pasa a `"03.11"` (v3.1.1).
- `template.xml` no cambia: el estilo `a1`, `bg.png` y `startup.png` se mantienen.
- El Title ID sigue siendo `ELEVENMPV`. Por eso la instalación sobre la v3.1.0 es una actualización y conserva `ux0:data/ElevenMPV`.

### 5. Ícono: variante 1a, sin recorte circular propio

Se exporta `exp-icon0-a`: el cuadrado naranja completo con la nota. La consola aplica su propio recorte circular a la burbuja. Si el PNG ya trajera el círculo, quedaría un borde de otro color en las esquinas del recorte del sistema. La nota ocupa los 62 px centrales, dentro del círculo inscrito.

### 6. Estrategia de pruebas

- **Compilación**: `scripts/build.sh --clean` compila con `-Werror`. El `.vpk` contiene `sce_sys/pic0.png`, y el `param.sfo` tiene `TITLE` "Material Eleven" y `APP_VER` 03.11.
- **Chequeo en PC**: el chequeo de la decisión 3 pasa con las cuatro imágenes nuevas y falla con un PNG RGBA o de otro tamaño puesto en lugar de una de ellas. Cada PNG se compara a ojo contra el render del HTML antes de borrar `docs/assets`.
- **Consola** (los scenarios de `specs/ui/livearea`):
  - Instalar con VitaShell sobre la v3.1.0 con biblioteca y ajustes.
  - Revisar el nombre y el ícono en la pantalla de inicio, sobre un wallpaper claro y uno oscuro.
  - Abrir la LiveArea: revisar el fondo, el gate y el botón de inicio, y que la barra de progreso del fondo no choque con lo que dibuja el sistema.
  - Iniciar la app: ver el splash.
  - Confirmar que la biblioteca y los ajustes se conservan.
- **Cache de LiveArea**: si después de actualizar la consola sigue mostrando el arte viejo, se prueba primero reiniciar la consola, y como último recurso reconstruir la base de datos. Si hace falta alguna de las dos cosas, se documenta en las notas de la tarea de consola.

### 7. CI

El pipeline no cambia: `build.yml` compila el `.vpk` con los PNG ya versionados.

### 8. Licencias

- **El arte nuevo** es propio del proyecto, bajo GPL-3.0-or-later, como el resto del repositorio.
- **El texto de las imágenes** se renderiza con Manrope e IBM Plex Mono, ambas bajo SIL OFL 1.1, ya registradas en `NOTICE` y en `res/OFL-*.txt`. La OFL no restringe las imágenes producidas con la fuente.
- **Herramientas**: `pngquant` (GPL-3.0) y el navegador solo se usan una vez para generar el arte. No se versionan ni se distribuyen.
- **Créditos**: el banner de Preetisketch deja de distribuirse, así que su línea sale de `NOTICE` (sección ARTWORK AND DESIGN) y de los créditos de `README.md`. La línea de LineageOS se mantiene, porque se refiere a elementos de diseño de la interfaz.

## Risks / Trade-offs

- [Headless no respeta `--window-size` exacto en alguna versión del navegador (barra de scroll, escala de DPI)] → Se usan `--hide-scrollbars` y `--force-device-scale-factor=1`, y el chequeo de la decisión 3 verifica el tamaño exacto, así que una captura desfasada se detecta antes de versionarla.
- [La cuantización cambia visiblemente un color de la paleta] → Con alrededor de 15 colores planos, `pngquant` los conserva exactos. Se comparan los PNG generados contra el render del HTML antes de instalar.
- [Las fuentes no cargan y el texto sale con una fuente de respaldo] → Las rutas de fuente se apuntan a `res/` en la página temporal, y la comparación a ojo contra el render del HTML lo detecta.
- [Borrar `docs/assets` antes de tener los PNG buenos pierde el diseño, porque no está en git] → El borrado es la última tarea de la generación, después del chequeo de formato y de la comparación a ojo.
- [Cambiar el arte más adelante sin la fuente en el repo] → Se asume: se vuelve a exportar desde la herramienta de diseño y se reemplazan los PNG, con el mismo chequeo.
- [La LiveArea sigue mostrando el arte viejo por el cache] → Se cubre en la prueba de consola de la decisión 6.

## Migration Plan

- Para el usuario: instalar el `.vpk` encima de la versión anterior. El Title ID es el mismo, y los datos de `ux0:data/ElevenMPV` no se tocan.
- Rollback: reinstalar el `.vpk` anterior restaura el arte y el nombre viejos, sin efectos sobre los datos.
- Rama: `change/refresh-livearea` sale de `main`, que ya tiene `add-language` integrado.
