## 1. Preparar el árbol en `vitasdk-legacy`

- [x] 1.1 Resolver los cambios sin commit en `.claude/CLAUDE.md` y `AGENTS.md`, y hacer commit de este cambio de OpenSpec (proposal, design, tasks, `.openspec.yaml`). Listo cuando `git status` esté limpio.
- [x] 1.2 Confirmar la variante de GPLv3 de ElevenMPV-A, "only" u "or later", revisando su `LICENSE` y las cabeceras de sus fuentes en `master`. Registrar el resultado en design.md (D5). Listo cuando D5 cite la evidencia.
- [x] 1.3 Inventariar las bibliotecas de `libs/`: para mpg123, dr_libs, libogg, libvorbis, libopus y libxmp-lite, anotar la versión (desde los headers o el `.a`) y la licencia. Listo cuando cada una tenga licencia identificada o esté marcada como "no identificada".
- [x] 1.4 Commit de licencia: `LICENSE` pasa a GPL-3.0, el texto Apache-2.0 va a `LICENSES/Apache-2.0.txt`, las licencias identificadas en 1.3 van a `LICENSES/` y se crea el `NOTICE` según D5. Listo cuando el NOTICE nombre a Joel16, a GrapheneCt/ElevenMPV-A (con `vitaaudiolib.c` y el menú de EQ), cada componente de `libs/`, las fuentes OFL, el banner de Preetisketch y LineageOS Eleven.
- [x] 1.5 Commit del README según D6: título Material-Eleven, aviso de fork modificado con el resumen de cambios, sección de licencia y créditos ampliados. Listo cuando el README enlace a `joel16/ElevenMPV` y a `GrapheneCt/ElevenMPV-A`.
- [x] 1.6 Commit de `.gitignore` y `.claude/` según D9. Listo cuando `git ls-files .claude` salga vacío y `.claude/CLAUDE.md` siga existiendo en disco.
- [x] 1.7 Commit de `openspec/config.yaml` según D7. Listo cuando `openspec context --json` devuelva el contexto nuevo sin errores y `openspec validate migrate-to-material-eleven` pase.
- [ ] 1.8 Commit de versión según D8: `VITA_VERSION "03.00"` y un `CHANGELOG.md` nuevo con la sección `## [3.0.0]` que lleve las notas de versión de la propuesta. Listo cuando el `.vpk` compile en WSL con VitaSDK y el `param.sfo` generado tenga `APP_VER 03.00`.
- [ ] 1.9 Verificar que ninguno de los commits de este grupo tenga trailer de co-autor: `git log 6476e2e..HEAD --format=%B | grep -ci anthropic` debe dar 0.

## 2. Respaldo y reescritura (fuera del repo, sin publicar nada)

- [ ] 2.1 Instalar `git filter-repo`, por ejemplo con `pip install git-filter-repo`. Listo cuando `git filter-repo --version` responda.
- [ ] 2.2 Crear el respaldo `git bundle create <fuera-del-repo>/ElevenMPV-pre-migracion.bundle --all`. Listo cuando `git bundle verify` sobre el archivo pase.
- [ ] 2.3 Clonar en el scratchpad con `--no-local`, trayendo solo `vitasdk-legacy` y los tags `v1.00`–`v2.10`. Listo cuando `git branch -a` y `git tag` del clon muestren solo eso.
- [ ] 2.4 Ejecutar `filter-repo` con el `--message-callback` de D2 y renombrar la rama a `main`. Listo cuando `git log main --format=%B | grep -ci anthropic` dé 0 y `git diff <hash-original-de-vitasdk-legacy> main` salga vacío (se compara contra el repo original agregado como remoto temporal).
- [ ] 2.5 Revisar que los commits anteriores a `3ea284a` conserven su hash, por ejemplo `65d2fca` y `11a7ec2`, y que los tags apunten a los mismos commits. Listo cuando `git rev-parse v2.10^{commit}` coincida con el original.

## 3. Hashes de los documentos archivados

- [ ] 3.1 Escribir en el scratchpad el script de D3: lee `.git/filter-repo/commit-map` y reemplaza en `openspec/changes/archive/**/*.md` solo los tokens entre backticks que sean prefijo único de un hash cambiado, conservando el largo abreviado. Listo cuando una ejecución en seco liste cada reemplazo con su archivo y línea.
- [ ] 3.2 Aplicar el script en el clon, revisar con `git diff --word-diff` y hacer commit con "Point archived records at the rewritten history". Listo cuando cada hash citado en los archivos archivados resuelva con `git rev-parse --verify <hash>^{commit}` en el clon.

## 4. Publicar el repo nuevo (requiere confirmación del usuario)

- [ ] 4.1 Con confirmación explícita, crear `Perroabuelo/Material-Eleven` (público) con `gh repo create`, hacer push de `main` y de los tags `v1.00`–`v2.10`. Listo cuando `gh repo view Perroabuelo/Material-Eleven --json defaultBranchRef,isFork` muestre `main` e `isFork: false`.
- [ ] 4.2 Poner la descripción del repo y su licencia visible. Listo cuando GitHub detecte "GPL-3.0" en la página del repo.
- [ ] 4.3 Clonar desde GitHub en un directorio limpio y compilar el `.vpk`. Listo cuando la compilación termine sin errores.

## 5. Repuntar el repo local

- [ ] 5.1 En el repo local: `origin` pasa a apuntar a Material-Eleven y `upstream` a `joel16/ElevenMPV`, con `fetch`. `main` local sigue a `origin/main` y el árbol queda igual al de `origin/main`. Listo cuando `git status -sb` muestre `## main...origin/main` sin diferencias.
- [ ] 5.2 Borrar las ramas locales `master` y `vitasdk-legacy`, y los tags locales que no se publicaron (`v3.0`–`v7.10`). Listo cuando `git branch` muestre solo `main` y `git tag` solo `v1.00`–`v2.10`.
- [ ] 5.3 Reindexar Repowise sobre el historial nuevo. Listo cuando el bloque de `AGENTS.md` cite un commit de `main`.

## 6. Retirar el fork viejo y etiquetar (requiere confirmación del usuario)

- [ ] 6.1 Con confirmación explícita y el bundle de 2.2 verificado, ejecutar `gh auth refresh -s delete_repo` y luego `gh repo delete Perroabuelo/ElevenMPV`. Listo cuando `gh repo view Perroabuelo/ElevenMPV` responda que no existe.
- [ ] 6.2 Crear el tag anotado `v3.0.0` sobre `main` y publicarlo. Crear el GitHub Release con el `.vpk` compilado en 4.3 adjunto. Listo cuando el Release muestre el `.vpk` descargable.
- [ ] 6.3 Renombrar el directorio según D10. Lo hace el usuario con la sesión cerrada: `Proyectos/ElevenMPV` pasa a `Proyectos/Material-Eleven` y `~/.claude/projects/C--Users-fotos-Documents-Proyectos-ElevenMPV` pasa a `…-Material-Eleven`. También se actualiza `SRC_DIR` en `scripts/build.sh` (que es local y no se versiona), porque tiene la ruta vieja fija. En la sesión nueva se reindexa Repowise. Listo cuando la sesión nueva recuerde la memoria del proyecto y `git status` esté limpio.
- [ ] 6.4 Archivar este cambio (`openspec archive migrate-to-material-eleven`) en una rama `change/…` con su PR, que es el primer uso del flujo nuevo. Listo cuando el PR esté fusionado en `main`.
