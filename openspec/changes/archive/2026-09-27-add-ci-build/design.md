## Context

La motivación está en proposal.md, en la sección Why. Estos son los hechos del árbol y del entorno que condicionan el diseño:

- El build local usa CMake 3.28.3 y `arm-vita-eabi-gcc` 15.2.0 (VitaSDK en WSL, Ubuntu 24.04), con `-O3 -g -Wall -Werror -fcommon` (`CMakeLists.txt:22`). El producto es `ElevenMPV.vpk` porque `project(ElevenMPV)`, y cambiar ese nombre queda fuera de alcance porque es identidad de la app.
- `CMakeLists.txt` empieza con `cmake_minimum_required(VERSION 2.8)`. CMake 3.28 solo lo advierte, pero CMake 4 elimina la compatibilidad con versiones menores a 3.5 y falla al configurar.
- Las bibliotecas que se enlazan desde VitaSDK (vita2d, freetype, png, jpeg, z, bz2, FLAC, vorbis, vorbisfile, ogg, mpg123) son paquetes de vdpm. Opus, opusfile y xmp-lite vienen en `libs/`.
- La imagen oficial `vitasdk/vitasdk` (repo `vitasdk/docker`) usa Ubuntu 24.04. Sus tags son series como `2026.08`. La variante completa trae todos los paquetes de la serie y la `-minimal` no trae ninguno.
- `VITA_VERSION` sigue la regla `vX.Y.Z` → `XX.YZ` definida en `config.yaml` (hoy `03.00`). `CHANGELOG.md` usa encabezados `## [X.Y.Z] - AAAA-MM-DD`.
- El repo es público, así que los minutos de Actions no tienen costo.

## Goals / Non-Goals

**Goals:**
- Un check de compilación con un nombre estable, que la protección de `main` pueda exigir.
- Que el CI y el build local compilen con el mismo resultado desde el mismo árbol.
- Releases reproducibles: el `.vpk` del Release sale del commit del tag, y el tag no se publica si su versión no coincide con `VITA_VERSION` o con `CHANGELOG.md`.
- Que la lógica de verificación se pueda probar en local, sin depender de GitHub.

**Non-Goals:**
- Caché de compilación. El proyecto compila en menos de un minuto y el costo lo pone la descarga de la imagen.
- Una matriz de versiones de VitaSDK.

## Decisions

### D1. La imagen es `vitasdk/vitasdk:<serie>`, completa y fijada
Se usa como `container:` del job, fijada a una serie concreta (`2026.08` o la vigente al implementar) y no a `latest`. Así, un cambio del toolchain no rompe PRs que no tocaron nada. Actualizar la serie es un cambio deliberado de una línea.
*Inspección (2026-09-27):* la serie elegida es `2026.08` (digest `sha256:fd82d88ff456d118eef16533ebf9a3a6ae0e292684fb8a3a35f15f0707b4692d`, publicada el 2026-09-25). Trae Ubuntu 24.04.5, CMake 3.28.3 y `arm-vita-eabi-gcc` 15.2.0, igual que el build local. `$VITASDK` es `/usr/local/vitasdk` y el job corre como `root`. `libvita2d.a`, `libFLAC.a`, `libmpg123.a`, `libvorbisfile.a` y `libfreetype.a` están en `$VITASDK/arm-vita-eabi/lib/`, y la imagen trae `make` y `git`. El árbol compila en el contenedor sin errores: da un `.vpk` de 2 053 841 bytes (2 053 502 en local) con `APP_VER 03.00` y `TITLE_ID ELEVENMPV`. La serie se reconstruye con sufijos fechados (`2026.08-20260925`), pero el toolchain de la serie no cambia.

*Alternativas descartadas:* instalar VitaSDK en cada run con `vdpm`, que es más lento y más frágil. Usar la variante `-minimal` más `vdpm` de los paquetes, que es más ligera pero obliga a mantener a mano la lista de paquetes. `latest`, que no es reproducible.

### D2. Un workflow reutilizable de build y otro de release que lo llama
`build.yml` se dispara con `pull_request` (hacia `main`), `push` (a `main`) y `workflow_call`. Tiene un solo job, `build`, que:
1. Hace checkout.
2. Configura con `cmake -S . -B build` (el toolchain sale de `$VITASDK`, que ya viene definido en la imagen).
3. Compila con `make -C build -j$(nproc)`.
4. Sube `build/ElevenMPV.vpk` como artefacto.

`release.yml` llama a `build.yml` con `uses: ./.github/workflows/build.yml`. Luego, en un job aparte con `permissions: contents: write`, descarga el artefacto y crea el Release. Así la compilación está definida una sola vez.
*Alternativa descartada:* un único workflow con condiciones por evento. Mezcla permisos de escritura con runs de PR y hace confuso el nombre del check.

### D3. La verificación vive en scripts versionados en `.github/scripts/`
- `check-version.sh <tag>` valida que el tag sea `vX.Y.Z` con Y y Z menores o iguales a 9, calcula `XX.YZ` y lo compara con `VITA_VERSION` en `CMakeLists.txt`. Si algo no calza, termina con un código distinto de cero y un mensaje claro.
- `changelog-section.sh <X.Y.Z>` imprime la sección `## [X.Y.Z]` de `CHANGELOG.md`, hasta el encabezado `## [` siguiente, y falla si la sección no existe o está vacía.

Son bash puro sobre coreutils, así que corren igual en el contenedor, en WSL y en Git Bash. `.gitattributes` fija `*.sh` en LF, porque con `core.autocrlf` un checkout en Windows los dejaría en CRLF y bash fallaría al correrlos desde WSL. Se prueban en local con casos buenos y malos (ver la estrategia de pruebas).
*Alternativa descartada:* poner esa lógica inline en el YAML, donde no se puede probar sin empujar tags.

### D4. El Release se crea con `gh` y solo se usan acciones de GitHub
El job de release ejecuta `gh release create vX.Y.Z Material-Eleven-X.Y.Z.vpk --title "Material-Eleven X.Y.Z" --notes-file <sección>` con `GITHUB_TOKEN`. El asset se renombra a `Material-Eleven-X.Y.Z.vpk` sin tocar `project()`. Solo se usan `actions/checkout`, `actions/upload-artifact` y `actions/download-artifact`, sin acciones de terceros.
*Por qué:* limita la cadena de suministro a GitHub y a la imagen de VitaSDK.
*Nota:* el Release `v3.0.0`, publicado a mano antes de este cambio, usó `Material-Eleven-v3.0.0.vpk`. Desde la próxima versión, el nombre es el de este workflow, sin la `v`.

### D5. `release.yml` también acepta un `workflow_dispatch` en seco
Con la entrada `tag` y `dry_run: true`, el workflow compila, ejecuta las dos verificaciones y sube el artefacto, pero no crea el Release. Sirve para probar el workflow antes del primer tag real. Hace falta porque GitHub no permite ejecutar un workflow disparado por tag sin empujar el tag.
GitHub solo acepta el `workflow_dispatch` cuando el workflow ya está en `main` (antes responde HTTP 404). Por eso, antes del merge, el dry run se hace con un disparador `pull_request` temporal que fija `tag` y `dry_run`, y que se revierte en seguida.
Un run manual que no sea en seco solo publica si corre sobre `refs/tags/<tag>`, para que el asset salga siempre del commit del tag.

### D6. `main` se protege con un ruleset
El ruleset exige PR para actualizar `main` (sin exigir aprobaciones, porque hay un solo mantenedor), exige el check `build / build` en verde y con la rama al día, y bloquea el force push y el borrado de la rama. No hay bypass: el flujo del config también vale para el dueño. Se aplica con `gh api` después de que el primer run del check exista, porque GitHub solo ofrece como requeridos los checks que ya corrieron.
*Consecuencia:* desde que se aplica, todo cambio entra por PR, también los del dueño.

### D7. Solo se sube el mínimo de CMake si la imagen lo exige
Si la imagen trae CMake 4 o superior, `cmake_minimum_required(VERSION 2.8)` pasa a `VERSION 3.10`, que es compatible con 3.28 local y quita la advertencia actual. Si trae CMake 3.x, no se toca, porque no es necesario para este cambio.
*Resultado (1.1):* `vitasdk/vitasdk:2026.08` trae CMake 3.28.3, así que `CMakeLists.txt` no se toca. La advertencia de compatibilidad sigue saliendo en el CI y en local, pero no es un error.

## Risks / Trade-offs

- [El GCC de la imagen emite un warning que el GCC local no emite, y `-Werror` lo convierte en error] → Se fija la serie (D1). Si al implementar aparece un warning nuevo, se corrige en el código dentro de este cambio cuando sea trivial. Si no lo es, se pausa para decidir.
- [Descargar la imagen completa, unos 1.6 GB, alarga cada run] → Es aceptable para un repo con pocos PRs. Si molesta, se puede pasar a `-minimal` más los paquetes de vdpm en un cambio aparte.
- [La protección de `main` bloquea al dueño si el CI se rompe por algo externo, como una imagen que desaparece] → Como la serie está fijada, eso solo pasa si se borra la imagen. En ese caso se puede desactivar el ruleset temporalmente desde la configuración del repo.
- [Un tag con la versión equivocada] → `check-version.sh` falla antes de publicar y no se crea el Release. El tag se borra y se vuelve a crear.
- [`$VITASDK` o las rutas de la imagen difieren de lo esperado] → La primera tarea de implementación inspecciona la imagen (`cmake --version`, `$VITASDK`, las bibliotecas presentes) antes de escribir el workflow.

## Estrategia de pruebas

- **Scripts (D3):** probarlos en local con bash. `check-version.sh` con `v3.0.0` (pasa), `v3.0.1` (falla porque no calza), `v3.10.0` (falla porque Y es mayor que 9) y `3.0.0` (falla porque falta la `v`). `changelog-section.sh` con `3.0.0` (imprime la sección) y `9.9.9` (falla).
- **Build (D2):** el PR de este mismo cambio tiene que quedar en verde, y el `.vpk` del artefacto tiene que coincidir en tamaño aproximado y `APP_VER` con el local. Además, un commit temporal en una rama desechable con un warning deliberado tiene que poner el check en rojo. Después se borra la rama.
- **Release (D5):** un `workflow_dispatch` en seco con `tag=v3.0.0` tiene que quedar en verde sin crear el Release. `v3.0.0` ya existe (se publicó a mano), así que el dry run usa ese tag solo para verificar la versión y las notas. El primer Release real del workflow será la próxima versión.
- **Protección (D6):** después de aplicarla, un `git push origin main` directo tiene que ser rechazado.

## Cambios al pipeline de CI

Este cambio crea el pipeline: `build.yml` (PR y main) y `release.yml` (tags y ejecución manual en seco). No hay tests en PC que agregar todavía.

## Open Questions

- ~~¿Cuál es la serie exacta de la imagen?~~ Resuelta en 1.1: `2026.08` (ver D1).
