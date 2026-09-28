## 1. Generación del arte

- [x] 1.1 Generar `sce_sys/icon0.png` (variante 1a), `sce_sys/livearea/contents/bg.png`, `sce_sys/livearea/contents/startup.png` y `sce_sys/pic0.png` desde el HTML de `docs/assets`, según la decisión 2: recorte por id, captura headless al tamaño exacto con las fuentes de `res/`, y `pngquant 256 --nofs --strip`. Las herramientas quedan fuera del repositorio. Listo cuando existan los cuatro PNG en `sce_sys/`.
- [x] 1.2 Verificar los cuatro PNG según la decisión 3: firma PNG, color type 3, bit depth 8 y tamaño exacto, y que el mismo chequeo falle con un PNG RGBA de prueba puesto en lugar de uno. Comparar cada PNG a ojo contra el render del HTML. Versionar los PNG. Listo cuando el chequeo pase, la comparación no muestre diferencias de color ni de posición, y los cuatro PNG estén en un commit.
- [x] 1.3 Borrar `docs/assets/` (el HTML, `support.js` y `.thumbnail`). `docs/screenshot/` se queda para el cambio del README. Listo cuando `docs/assets/` no exista y `git status` no muestre nada de él.

## 2. Empaquetado y créditos

- [x] 2.1 En `CMakeLists.txt`, cambiar `VITA_APP_NAME` a `"Material Eleven"` y agregar `FILE sce_sys/pic0.png sce_sys/pic0.png` a `vita_create_vpk`. Listo cuando `scripts/build.sh --clean` compile con `-Werror`, el `.vpk` contenga `sce_sys/pic0.png` y el `param.sfo` generado tenga `TITLE` "Material Eleven".
- [x] 2.2 Quitar el crédito del banner de Preetisketch de `NOTICE` (ARTWORK AND DESIGN) y de los créditos de `README.md`, y mantener la línea de LineageOS. Listo cuando `grep -ri preetisketch` no devuelva nada en el repo, salvo en los artefactos archivados de OpenSpec.

## 3. Verificación en consola

- [x] 3.1 Instalar el `.vpk` con VitaShell sobre la v3.1.0, con una biblioteca escaneada y ajustes cambiados, y recorrer los scenarios de `specs/ui/livearea`:
  - Nombre e ícono en la pantalla de inicio, sobre un wallpaper claro y uno oscuro.
  - LiveArea con el fondo, el gate y el botón de inicio, y la barra de progreso del fondo sin choque con lo que dibuja el sistema.
  - Splash al iniciar.
  - Biblioteca y ajustes conservados.

  Anotar en esta tarea si hizo falta reiniciar la consola o reconstruir la base de datos para ver el arte nuevo. Listo cuando todos los scenarios se cumplan en consola.

  Resultado: todos los scenarios se cumplen en consola. El arte nuevo apareció al instalar, sin reiniciar la consola ni reconstruir la base de datos.

## 4. Cierre

- [x] 4.1 Agregar a `CHANGELOG.md`, en `[Sin publicar]` y bajo `### Cambiado`, los bullets de las notas de versión de la propuesta, y subir `VITA_VERSION` a `"03.11"` en `CMakeLists.txt`. Listo cuando `changelog-section.sh "Sin publicar"` imprima los bullets, `check-version.sh v3.1.1` pase y el `.vpk` compilado tenga `APP_VER 03.11`.
- [x] 4.2 Ejecutar `openspec validate refresh-livearea --strict`. Listo cuando pase y el check `build` del PR de `change/refresh-livearea` hacia `main` esté en verde.
