## Context

El porqué está en proposal.md. Acá va el estado que condiciona el cómo.

- **README actual.** Arriba muestra `sce_sys/pic0.png` y, bajo `# Screenshots:`, una tabla de 3×2 con capturas de `docs/screenshots/`. En esa carpeta hay 13 JPG de versiones anteriores, y las revisiones viejas del README los enlazan por esa ruta.
- **Material de la 3.5.** Son 29 capturas de la consola (960×544) y una grabación de OBS de 2:49, en 1920×1080 a 60 fps, ambas fuera del repo.
  - **El audio de la grabación está en silencio** (−91 dB constantes). La captura de la Vita no lleva sonido, así que la música se agrega después, desde los archivos originales.
  - **Los colores de la grabación están mal etiquetados.** OBS guardó píxeles de rango completo (0–255) en un archivo marcado como rango limitado (16–235), y los reproductores aplastan los negros. El fondo de la app se ve `RGB(1,1,8)` en vez de `RGB(18,15,22)`. Reinterpretarlo como rango completo (`setparams=range=pc`) devuelve `RGB(17,16,23)`, lo mismo que en las capturas.
  - **La nitidez tiene un techo.** La consola dibuja a 960×544 y OBS escaló a 1080p con un filtro suave, así que el video no puede tener más detalle que eso.
- **Videos en GitHub.** Un `.mp4` guardado en el repo no se reproduce dentro de un README. Para que se reproduzca hay que subirlo desde el editor web (en un issue, en un PR o al editar un `.md`): GitHub responde con una URL `github.com/user-attachments/assets/...`, y el archivo no puede pesar más de 10 MB en un plan gratuito. Esa subida no tiene API pública.
- **Herramientas.** `tools/` ya existe (`dev-safety-net`). Sus herramientas usan solo la biblioteca estándar de Python 3, no tienen rutas de una máquina, y sus herramientas externas son opcionales.
- **El prototipo existe.** El montaje de la 3.5 se hizo con un `build.py` fuera del repo, con rutas fijas y la edición escrita dentro del código. Sus salidas son las que se describen abajo.

## Goals / Non-Goals

**Goals:**
- Que el README muestre la app de la 3.5 en movimiento, sin depender de un servicio externo.
- Que rehacer el video para otra versión sea grabar, escribir un archivo de edición y correr un comando.
- Que las capturas de cada versión queden en el repo y se puedan enlazar.

**Non-Goals:**
- Un editor de video general. `build.py` hace una sola cosa: el formato de este video, que es la pantalla enmarcada con títulos, el montaje de colores, las letras y el cierre.
- Corregir el color de grabaciones que ya vienen bien etiquetadas. La corrección se activa con una opción.

## Decisions

### 1. El video del README se sube a GitHub y no se versiona

El README lleva la URL `user-attachments` sola en una línea, que GitHub muestra como reproductor. Se sube la versión de GitHub **con audio**: GitHub la reproduce sin sonido hasta que el visitante lo activa. La versión muda queda para los sitios que reproducen solos.

- **Alternativa descartada: guardar el `.mp4` en `docs/media/`.** GitHub lo muestra como un enlace de descarga, no como un reproductor. Además, metería en la historia del repo canciones con derechos de autor.
- **Alternativa descartada: solo un GIF.** No tiene audio, pesa más por segundo y se ve con bandas de color. Queda como respaldo.
- **Alternativa descartada: YouTube con una miniatura enlazada.** Saca al visitante de la página, y Content ID reclamaría las canciones.

Hasta que el autor sube el video, el README muestra el GIF en esa posición. La tarea de reemplazarlo es manual y queda en tasks.md.

### 2. Las capturas se agrupan por versión

Las capturas nuevas van a `docs/screenshots/v3.5/`. Las 13 actuales no se mueven: si se movieran, las revisiones viejas del README, que las enlazan por ruta, mostrarían imágenes rotas. La próxima versión agrega `v3.6/`, y así sucesivamente.

Esta es la selección de la 3.5: dos columnas, cuatro filas, cada una con su texto alternativo.

| Archivo nuevo | Captura original | Qué muestra |
|---|---|---|
| `now-playing-pink.jpg` | `2026-10-08-210047.jpg` | Now playing, acento rosa (VILLAIN DIES) |
| `now-playing-cyan.jpg` | `2026-10-08-210215.jpg` | Now playing, acento cian (Wish) |
| `now-playing-green.jpg` | `2026-10-08-210313.jpg` | Now playing, acento verde (Wanna Go Back) |
| `now-playing-blue.jpg` | `2026-10-08-210343.jpg` | Now playing, acento azul (Supernatural) |
| `lyrics.jpg` | `2026-10-08-210158.jpg` | Letras sincronizadas (Eyes Roll) |
| `library-artist-mini-player.jpg` | `2026-10-08-210204.jpg` | Dentro de un artista, con el mini player |
| `folders-lrc.jpg` | `2026-10-08-205955.jpg` | Navegador de carpetas con los `.lrc` |
| `settings-playback.jpg` | `2026-10-08-205938.jpg` | Ajustes, qué hacer al terminar un álbum |

Se copian sin recomprimir.

### 3. `build.py` separa la herramienta de la edición

- **`tools/promo/build.py`** sabe armar el video. Recibe por argumentos la grabación, la música de fondo, la canción de la escena de letras, el avatar (opcional) y la carpeta de salida. Las fuentes las toma de `res/`.
- **`tools/promo/edits/v3.5.json`** describe una grabación:
  - cada escena con su tramo de la grabación, su duración y su velocidad;
  - el título, el subtítulo y el color de acento;
  - qué escenas forman el montaje de colores y cuál es la de letras;
  - el segundo en que empezó la canción de las letras;
  - el segundo de la caída del beat de la música de fondo;
  - el texto del cierre.
- **Salidas:** `github` (1080p30, unos 28 s, menos de 10 MB) y `full` (1080p60, unos 55 s), cada una con y sin audio, más un GIF y un WebP de 640×360, mudos y en loop.

Cómo se arma cada escena:
- Se corrige el rango de color (con la opción `--fix-obs-range`), se enmarca la pantalla en 1440×810 con esquinas redondeadas sobre un fondo oscuro con un resplandor del color de acento, y se dibujan el título y el subtítulo en Manrope.
- Las escenas se unen con fundidos cortos.
- La música de fondo arranca de modo que su caída del beat coincide con el primer corte después de la intro. Bajo la escena de letras entra la canción que suena en pantalla, en el mismo punto que marca el contador de la app, para que las letras coincidan con la voz.
- El audio se normaliza a −15 LUFS.

Las escenas intermedias se guardan en una carpeta de trabajo, con una clave que depende de su configuración. Así, al cambiar una escena solo se vuelve a renderizar esa.

- **Alternativa descartada: dejar la edición dentro de `build.py`.** Cada versión tendría que editar código, y las rutas de una máquina terminarían en el repo.
- **Alternativa descartada: no versionar la herramienta.** Es lo que pasó con `psp2dmp` antes de `dev-safety-net`: se pierde.

### 4. La guía de grabación va en `tools/README.md`

`tools/README.md` suma una sección para `promo/build.py`, igual que las otras herramientas. Explica:
- Qué necesita: Python 3 y ffmpeg con libx264, libwebp y drawtext.
- Cómo correrlo, y cómo leer los tiempos de la grabación con una hoja de contactos de ffmpeg.
- Cómo ajustar OBS para no necesitar la corrección:
  - **Rango de color:** en Ajustes > Avanzado, y que coincida con el de la fuente de captura.
  - **Lienzo:** 1920×1088, que es exactamente 2× la pantalla de la consola.
  - **Filtro de escalado:** "Point" en la fuente, para que cada píxel de la consola sea un bloque nítido de 2×2.
  - **Audio:** grabarlo aparte, porque la captura no lo trae.

### 5. Estrategia de pruebas

No hay código C ni cambios en la app, así que el `.vpk` y los tests en PC no cambian. El CI los corre igual en el PR.

- **`build.py`:** con la grabación de la 3.5 y `edits/v3.5.json`, tiene que producir las seis salidas. Se verifica:
  - La duración de cada una, con un margen de ±0.2 s respecto de la del prototipo: 28.0 s la de GitHub y 54.9 s la completa.
  - Que la de GitHub pese menos de 10 MB.
  - Que las versiones mudas no tengan pista de audio.
  - Revisándolas a ojo, que el color del fondo de la app sea `RGB(17,16,23)` ±2.
- **Sin rutas locales:** `git grep -n "C:/Users\|/c/Users"` sobre `tools/` no encuentra nada.
- **README:** todas las imágenes que enlaza existen en el repo. Lo último se verifica en la vista del README de la rama en GitHub: la tabla se ve bien, el GIF se mueve y, después de subir el video, el reproductor aparece.

### 6. CI

Sin cambios.

### 7. Licencias y assets de terceros

- **Fuentes:** Manrope e IBM Plex Mono vienen de `res/` y ya están registradas en `NOTICE` (SIL OFL 1.1, compatible con GPLv3).
- **Capturas y video:** muestran portadas de álbumes, igual que las capturas actuales.
- **Música:** no entra al repo. El autor acepta usar canciones con derechos en el video de GitHub. Si publica la versión completa en YouTube, Content ID puede reclamarla.
- **Avatar:** es arte propio del autor. Se pasa por argumento y no se versiona.

## Risks / Trade-offs

- [La URL `user-attachments` deja de funcionar si GitHub cambia cómo guarda los adjuntos] → El GIF de `docs/media/` sigue en el repo, y volver a él es cambiar una línea.
- [Las canciones del video tienen derechos de autor] → No se versionan. El video de GitHub es corto y no es comercial. Si llegara un reclamo, se reemplaza por la versión muda, que ya existe.
- [El archivo de edición depende de una grabación concreta] → Es lo esperado: cada versión tiene el suyo, y `v3.5.json` sirve de ejemplo.
- [El GIF suma alrededor de 1.1 MB a la historia del repo] → Es el único binario grande del cambio, y un WebP no se ve en todos los clientes que muestran el README.
