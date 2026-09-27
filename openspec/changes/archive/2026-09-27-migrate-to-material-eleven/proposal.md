## Why

El trabajo de la rama `vitasdk-legacy` ya es un proyecto propio: un skin inspirado en Material You, una biblioteca, una cola con orden de reproducción y un shuffle nuevo. Sin embargo, vive como fork de GitHub de `joel16/ElevenMPV` y su `master` es una copia de `GrapheneCt/ElevenMPV-A`, que es otra aplicación sin ningún archivo en común. Además, el commit `65d2fca` tradujo código de ElevenMPV-A, que es GPLv3, así que el proyecto ya no puede distribuirse como Apache-2.0. Hay que corregir esto antes de adoptar un flujo de trabajo con PRs, CI y versiones, porque ese flujo supone una rama `main` propia y un repo que no redirija los PRs al proyecto original.

## What Changes

- **BREAKING (repositorio):** el proyecto se muda a un repo propio, `Perroabuelo/Material-Eleven`, fuera de la red de forks. `vitasdk-legacy` pasa a ser `main`. El `master` de ElevenMPV-A no se lleva.
- Se reescribe el historial completo para quitar los trailers `Co-Authored-By: Claude …` (83 commits). Los hashes cambian desde el primer commit afectado, y las referencias a hashes en `openspec/changes/archive/*` se actualizan con el mapa de commits.
- De los tags heredados solo se conservan los que están en el historial de esta rama (`v1.00`–`v2.10`, de Joel16). Los tags de ElevenMPV-A no se llevan.
- **BREAKING (licencia):** el proyecto pasa de Apache-2.0 a GPLv3. Se conservan los avisos Apache del código original de Joel16. Se agregan un `NOTICE` y una sección de créditos que explican el origen de cada parte: Joel16/ElevenMPV como base, GrapheneCt/ElevenMPV-A por el audio portado, las bibliotecas vendorizadas, las fuentes OFL, el banner de Preetisketch y el diseño de LineageOS Eleven. También se inventarían las licencias de `libs/`.
- El README presenta el proyecto como **Material-Eleven**, un fork modificado de ElevenMPV, y dice qué cambió (Apache-2.0 §4b).
- `openspec/config.yaml` adopta la metodología de jeopardy adaptada a C, CMake y VitaSDK: una rama `change/<nombre>` por cambio, integración a `main` solo mediante PR, compilación del `.vpk` como validación, notas de versión, `CHANGELOG.md` y tags anotados. También se corrige la línea de idioma, que dice pt-BR cuando el contenido es español.
- `.claude/` deja de versionarse, como en jeopardy.
- Cuando el repo nuevo esté verificado, se borra el fork viejo `Perroabuelo/ElevenMPV`, que todavía publica el historial con los trailers.

## Capabilities

### New Capabilities

Ninguna. El cambio no modifica el comportamiento del reproductor, así que se declara `skip_specs: true`.

### Modified Capabilities

Ninguna.

## Fuera de alcance

- La identidad dentro de la app: `VITA_APP_NAME`, `project()`, el Title ID `ELEVENMPV`, el banner de LiveArea y la migración de los datos guardados. Queda para un cambio aparte, porque cambiar el Title ID convierte la app en otra para quien ya la tenga instalada.
- El CI (compilar el `.vpk` en GitHub Actions) y los tests unitarios en PC. Serán el cambio siguiente, `add-ci-build`, que es el primero en seguir el flujo nuevo.
- Reescribir el código portado de ElevenMPV-A para volver a Apache-2.0. Se descartó a favor de GPLv3.
- Cambios en el código del reproductor.

## Notas de versión

- Versión objetivo: **v3.0.0**, salto **major** (licencia nueva, repo nuevo y nombre nuevo). Queda por confirmar en design.md, que explica cómo se lleva a `APP_VER` `XX.YY`.
- Material-Eleven tiene su propio repositorio, separado del ElevenMPV original.
- El proyecto se distribuye bajo GPLv3, con créditos completos a ElevenMPV (Joel16), ElevenMPV-A (GrapheneCt) y las bibliotecas que usa.

## Impact

- **Repositorio:** todos los hashes cambian, los clones existentes quedan obsoletos, se crea un remoto `origin` nuevo, `upstream` apunta a `joel16/ElevenMPV` como referencia y se borra el fork viejo.
- **Archivos:** `LICENSE`, `NOTICE` (nuevo), `README.md`, `.gitignore`, `openspec/config.yaml`, `openspec/changes/archive/*` (referencias a hashes), `.claude/CLAUDE.md` (sale del índice de git) y, si se confirma el versionado, `VITA_VERSION` en `CMakeLists.txt`.
- **Código:** no cambia.
- **Herramientas:** requiere `git filter-repo` (hoy no está instalado) y `gh` autenticado con permisos para crear y borrar repos.
