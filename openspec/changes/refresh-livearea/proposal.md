Rama: `change/refresh-livearea`, creada desde `main` con `add-language` ya integrado. Su PR apunta a `main`.

## Why

Lo primero que se ve de la app en la consola sigue siendo la identidad de ElevenMPV:

- En la pantalla de inicio: la burbuja con la púa rosa, que dice "Eleven Music Player".
- En la LiveArea: un degradado gris azulado de fondo y el banner rosa "Eleven, Music Player For PS VITA" de Preetisketch.

Nada de eso coincide con la interfaz Material You que se ve al entrar, ni con el nombre del proyecto. En `docs/assets` ya hay un diseño nuevo, hecho con la paleta y la tipografía de la app, y conviene publicarlo antes de rehacer el README con capturas.

## What Changes

- **Nombre de la app**: la burbuja y la LiveArea pasan a decir **"Material Eleven"**, en vez de "Eleven Music Player".
- **Ícono (`icon0.png`)**: una nota musical blanca sobre el naranja de acento fijo de la app. Es la variante 1a del diseño.
- **Gate de la LiveArea (`startup.png`)**: el ícono en un recuadro redondeado junto al logotipo MATERIAL / Eleven y la línea "Music player · PS Vita".
- **Fondo de la LiveArea (`bg.png`)**: el fondo oscuro de la app, con ondas concéntricas que terminan en un punto de acento, y una barra de progreso como la de Reproduciendo.
- **Splash de arranque (`pic0.png`)**, que hoy no existe: el logotipo con "Music player for PS Vita" y los chips de los formatos soportados, con los colores de la app.
- **Los PNG son los assets versionados.** Se generan una sola vez desde el diseño de `docs/assets`, en el formato que exige la consola (PNG indexado de 8 bits, con el tamaño exacto de cada imagen), y quedan en `sce_sys/`. El diseño HTML y sus archivos de acompañamiento no se versionan y se borran de `docs/assets` después de generar.
- **Créditos**: salen de `NOTICE` y del README los créditos del banner de Preetisketch, que deja de distribuirse. El arte nuevo es propio del proyecto.

## Capabilities

### New Capabilities
- `ui/livearea`: cómo se presenta la app fuera de su propia interfaz. Cubre el nombre y el ícono en la pantalla de inicio, el fondo y el gate de la LiveArea, el splash de arranque, el formato de las imágenes que acepta la consola y la actualización sobre una instalación previa.

### Modified Capabilities
Ninguna. Los specs vigentes describen lo que pasa dentro de la app, y ninguno habla de su presentación en el sistema.

## Impact

- **Assets**:
  - Se reemplazan `sce_sys/icon0.png`, `sce_sys/livearea/contents/bg.png` y `sce_sys/livearea/contents/startup.png`, y se agrega `sce_sys/pic0.png`.
  - `template.xml` no cambia, porque los nombres de archivo se mantienen.
- **Diseño**: `docs/assets/` (el HTML, `support.js` y `.thumbnail`) se usa para generar los PNG y después se borra. No llega al repositorio.
- **Código y build**:
  - `CMakeLists.txt`: `VITA_APP_NAME` pasa a "Material Eleven", `vita_create_vpk` suma `pic0.png` y `VITA_VERSION` pasa a `"03.11"`.
  - El código C no cambia.
- **Herramientas**: generar los PNG necesita un navegador headless (Chromium o Edge) y `pngquant`, una sola vez y en la máquina del desarrollador. Nada de eso queda en el repositorio, y el CI y el `.vpk` no dependen de ellos.
- **Documentación**: la línea de créditos del banner en `NOTICE` y en `README.md`.
- **Instalaciones existentes**: el Title ID (`ELEVENMPV`) no cambia, así que el `.vpk` nuevo se instala encima del anterior y conserva los datos en `ux0:data/ElevenMPV`.

## Fuera de alcance

- El rediseño del README y las capturas nuevas, que van en un cambio aparte.
- Cambiar el Title ID o la ruta de datos (`ux0:data/ElevenMPV`).
- Otros estilos de LiveArea (`template.xml` sigue con el estilo `a1`), o sumar contenido a la LiveArea: texto, frames o enlaces.
- La variante 1b del ícono y cualquier ícono alternativo.
- Usar el nuevo logo dentro de la app.
- Generar el arte en CI.
- Una fuente versionada del arte o un script para regenerarlo: si el arte cambia más adelante, se vuelve a exportar y se reemplazan los PNG.

## Notas de version

Versión objetivo: **v3.1.1** (patch): este cambio no agrega funcionalidad, y la v3.1.0 ya se publicó con `add-language`.

- Nuevo ícono, nueva LiveArea y nueva pantalla de arranque, con el estilo de la app.
- La app aparece en la consola como "Material Eleven".
- Al instalar sobre la versión anterior se conservan tu biblioteca y tus ajustes.
