## 1. Herramienta del video

- [x] 1.1 Llevar el prototipo a `tools/promo/build.py`:
  - argumentos: `--recording`, `--bed`, `--lyrics-song`, `--avatar` (opcional), `--edit`, `--out`, `--fix-obs-range` y `--only github|full|gif`;
  - fuentes desde `res/` resueltas relativas al repo;
  - la carpeta de trabajo dentro de `--out`.

  **Listo cuando** `py -I tools/promo/build.py --help` muestra las opciones y `git grep -n "C:/Users\|/c/Users" tools/` no encuentra nada.
- [ ] 1.2 Escribir `tools/promo/edits/v3.5.json` con la edición de la 3.5:
  - las escenas y los tiempos del prototipo;
  - el inicio de TOMBOY en 150.35 s;
  - la caída del beat de Supernatural en 7.95 s;
  - el texto del cierre.

  **Listo cuando**, al correr con la grabación de la 3.5 y `--fix-obs-range`, salen las seis salidas (`github`, `github-muted`, `full`, `full-muted`, `.gif` y `.webp`). Además:
  - la de GitHub dura 28.0 s ±0.2 y pesa menos de 10 MB;
  - la completa dura 54.9 s ±0.2;
  - `ffprobe` no muestra pista de audio en las mudas;
  - en un fotograma de la biblioteca, el fondo de la app mide `RGB(17,16,23)` ±2.
- [ ] 1.3 Agregar la sección `promo/build.py` a `tools/README.md`: qué necesita, cómo correrlo, cómo leer los tiempos de una grabación con una hoja de contactos y los ajustes de OBS (rango de color, lienzo de 1920×1088, filtro "Point" y audio aparte). **Listo cuando** los comandos del README, copiados tal cual con las rutas de ejemplo reemplazadas, reproducen las salidas de 1.2.

## 2. Capturas y GIF

- [ ] 2.1 Copiar sin recomprimir las 8 capturas de la tabla de design.md a `docs/screenshots/v3.5/`, con sus nombres nuevos. **Listo cuando** `ls docs/screenshots/v3.5` muestra los 8 archivos, cada uno idéntico byte a byte a su original (mismo hash), y las 13 capturas anteriores siguen en `docs/screenshots/` sin cambios (`git status` no las lista).
- [ ] 2.2 Agregar `docs/media/material-eleven.gif`, generado en 1.2. **Listo cuando** el archivo existe, pesa menos de 2 MB y se reproduce en loop en el navegador.

## 3. README

- [ ] 3.1 Actualizar `README.md`:
  - bajo el banner, el GIF de `docs/media/` como lugar del video, con un comentario HTML que dice que se reemplaza por la URL `user-attachments`;
  - bajo `# Screenshots:`, la tabla de 2×4 con las capturas de `v3.5/` y su texto alternativo;
  - una línea que enlaza a `docs/screenshots/` para las versiones anteriores.

  **Listo cuando** cada ruta de imagen del README existe en el repo y la vista del README en la rama de GitHub muestra el GIF animado y la tabla completa.
- [ ] 3.2 (Manual, el autor) Subir `material-eleven-github.mp4` (con audio) desde el editor web de GitHub y reemplazar el GIF del README por la URL `user-attachments`, sola en su línea. **Listo cuando** la vista del README en la rama muestra el reproductor, el video se reproduce y el audio suena al activarlo.

## 4. Cierre

- [ ] 4.1 Hacer push de la rama y abrir el PR a `main`. **Listo cuando** el CI queda en verde (el `.vpk` y los tests en PC no cambian) y el README de la rama se ve completo en GitHub.
