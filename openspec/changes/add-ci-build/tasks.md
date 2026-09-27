## 1. Reconocer la imagen

- [x] 1.1 Inspeccionar `vitasdk/vitasdk:<serie>` (con `docker run` local o con un run temporal de `workflow_dispatch` en la rama del cambio): `cmake --version`, `arm-vita-eabi-gcc --version`, `echo $VITASDK` y la presencia de `libvita2d.a`, `libFLAC.a`, `libmpg123.a`, `libvorbisfile.a` y `libfreetype.a`. Listo cuando la serie elegida y los valores reales queden anotados en design.md (D1 y D7).

## 2. Compilación en CI

- [x] 2.1 Si 1.1 muestra CMake 4 o superior, cambiar `cmake_minimum_required` a `VERSION 3.10` (D7). Listo cuando `scripts/build.sh --clean` compile en local sin la advertencia de compatibilidad y el `.vpk` siga teniendo `APP_VER 03.00`. Si la imagen trae CMake 3.x, marcar la tarea como no necesaria, con la evidencia. No necesaria: la imagen `2026.08` trae CMake 3.28.3 (ver D7).
- [ ] 2.2 Crear `.github/workflows/build.yml` según D1 y D2: disparadores `pull_request` hacia `main`, `push` a `main` y `workflow_call`, un job `build` en la imagen fijada y el artefacto `ElevenMPV.vpk`. Listo cuando el push de la rama y el PR hacia `main` muestren el check `build` en verde y el artefacto descargado tenga `APP_VER 03.00`.
- [ ] 2.3 Probar que el check falla ante un warning. En una rama desechable creada desde `change/add-ci-build`, agregar una variable sin usar en `source/main.c`, empujar y ver el run en rojo por `-Werror`. Después borrar la rama local y la remota. Listo cuando el run rojo esté enlazado en esta tarea y la rama ya no exista.

## 3. Release

- [ ] 3.1 Crear `.github/scripts/check-version.sh` y `.github/scripts/changelog-section.sh` según D3. Listo cuando pasen en local (bash en WSL) los casos de la estrategia de pruebas: `v3.0.0` pasa, y fallan `v3.0.1`, `v3.10.0` y `3.0.0`; `3.0.0` imprime su sección y `9.9.9` falla.
- [ ] 3.2 Crear `.github/workflows/release.yml` según D2, D4 y D5: disparador `push` de tags `v*` y `workflow_dispatch` (`tag`, `dry_run`); llama a `build.yml`, ejecuta los dos scripts y crea el Release con `gh` y el asset `Material-Eleven-X.Y.Z.vpk`. Listo cuando un `workflow_dispatch` con `tag=v3.0.0 dry_run=true` sobre la rama quede en verde sin crear ningún Release (`gh release list` vacío).
## 4. Archivar, integrar y proteger `main` (requiere confirmación del usuario)

- [ ] 4.1 En `change/add-ci-build`, con el check en verde: agregar a `CHANGELOG.md`, en la sección `[3.0.0]` y bajo Agregado, el bullet de las notas de versión ("Cada versión publica su `.vpk` en la página de Releases de GitHub."). `VITA_VERSION` no se sube, porque 3.0.0 todavía no se publica. Luego ejecutar `openspec archive add-ci-build`. Listo cuando `changelog-section.sh 3.0.0` incluya el bullet, `openspec list` ya no muestre `add-ci-build` y el check del PR siga en verde.
- [ ] 4.2 Con confirmación explícita, fusionar el PR en `main`. Listo cuando el run de `build.yml` por el push a `main` quede en verde.
- [ ] 4.3 Con confirmación explícita, crear el ruleset de D6 sobre `main` con `gh api` (PR obligatorio, check `build` requerido, sin force push ni borrado y sin bypass). Listo cuando `gh api repos/Perroabuelo/Material-Eleven/rulesets` lo muestre activo y un `git push origin main` de prueba sea rechazado. El commit de prueba es un commit vacío local que después se descarta con `git reset`.
