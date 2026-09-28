# Registro de cambios

Todos los cambios visibles de la app se registran en este archivo.

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/) y el proyecto usa
[Versionado Semántico](https://semver.org/lang/es/). La versión `X.Y.Z` se muestra en la consola
como `XX.YZ` (por ejemplo, 3.0.0 es `03.00`).

## [Sin publicar]

### Agregado

- Cada versión publica su `.vpk` en la página de Releases de GitHub.
- La app ahora está disponible en inglés y en español.
- Al abrirla por primera vez, usa el idioma de tu consola: español si la consola está en español,
  inglés en cualquier otro caso.
- Nueva opción **Idioma** en Ajustes para elegir entre Sistema, English y Español, que se aplica
  al instante.

### Cambiado

- Los textos en español se revisaron: ortografía completa y español neutro.
- Actualizar a esta versión conserva todos tus ajustes.

## [3.0.0] - 2026-09-27

Primera versión de Material-Eleven, el fork de ElevenMPV 2.10 de Joel16.

### Agregado

- Interfaz nueva al estilo Material You, con un color de acento que se toma de la carátula.
- Biblioteca de música y una cola de reproducción, con shuffle y repeat independientes.
- Controles dibujados en vectores y tipografías nuevas (Manrope, IBM Plex Mono).
- El volumen sigue al control de volumen del sistema.
- Ajustes de audio con presets de EQ por hardware y un limitador opcional, traídos de ElevenMPV-A.

### Cambiado

- El proyecto tiene su propio repositorio, Perroabuelo/Material-Eleven.
- El proyecto se distribuye bajo GPL-3.0-or-later, con créditos completos a ElevenMPV (Joel16),
  a ElevenMPV-A (GrapheneCt) y a las bibliotecas que usa (ver NOTICE).
- El `.vpk` incluye, en `licenses/`, la licencia del proyecto, el NOTICE y las licencias de
  todas las bibliotecas y tipografías que lleva.
