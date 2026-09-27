## Context

La motivación está en proposal.md, en la sección Why. Estos son los hechos verificados sobre el árbol actual que determinan el enfoque:

- `origin` es `Perroabuelo/ElevenMPV`, un fork público de `joel16/ElevenMPV` cuya rama por defecto es `master`. `upstream` apunta a `GrapheneCt/ElevenMPV-A`. `master` es idéntico a `upstream/master` y no comparte ningún archivo con `vitasdk-legacy`. Las dos ramas se separaron en `11a7ec2` (2019-10-06): `vitasdk-legacy` tiene 146 commits propios y `master` tiene 86.
- En la rama hay 83 commits con el trailer `Co-Authored-By: Claude (Opus 5 | Opus 5.5 | Sonnet 5) <noreply@anthropic.com>`. El primero es `3ea284a`, así que los commits anteriores, incluidos `65d2fca` y todo el historial de Joel16, conservan su hash. Otros mensajes nombran `.claude/` o `CLAUDE.md` como archivos, y esos no se tocan.
- Los documentos en `openspec/changes/archive/*` citan hashes entre backticks, por ejemplo `f3d908e` y `341709b`, en 8 archivos. Algunos quedan antes de `3ea284a` y no cambian.
- De los tags, solo `v1.00`–`v2.10` están en el historial de la rama. `v3.0`–`v7.10` pertenecen a la línea de `master`.
- `LICENSE` es Apache-2.0. No hay `NOTICE` y ningún archivo de `source/` o `include/` tiene cabecera de copyright. Las fuentes OFL ya incluyen su licencia en `res/OFL-*.txt`. `libs/` tiene solo `include/` y `lib/` (binarios `.a` y headers) y ningún archivo de licencia.
- El código que exige GPLv3 está en `source/audio/vitaaudiolib.c`: grain configurable, volumen del sistema con `sceAppUtilSystemParamGetInt(9)` y limitador EQ. Es una traducción a C de `ElevenMPV-A/source/audio/vitaaudiolib.cpp`. El menú de EQ en `source/menus/menu_settings.c` probablemente también deriva de ahí. El read-ahead de `flac.c` se escribió de forma independiente.
- `.gitignore` versiona a propósito `.claude/CLAUDE.md` (regla `!.claude/CLAUDE.md`), además de `AGENTS.md` y `.codex/hooks.json`.
- `CMakeLists.txt` tiene `VITA_VERSION "02.10"`. El `APP_VER` del `param.sfo` tiene formato `XX.YY`.
- `git filter-repo` no está instalado. Git es 2.48.

## Goals / Non-Goals

**Goals:**
- Un repo `Perroabuelo/Material-Eleven` cuyo `main` tenga **el mismo árbol** que `vitasdk-legacy` después de los commits de preparación, y cuyos mensajes solo difieran por los trailers eliminados.
- Una migración que se pueda verificar antes de ejecutar cualquier paso irreversible, como publicar el repo nuevo o borrar el fork.
- Avisos de licencia y créditos que cumplan GPLv3 §4–5 y Apache-2.0 §4.
- Un `config.yaml` que sirva desde el primer cambio posterior (`add-ci-build`).

**Non-Goals:**
- Poner cabeceras de licencia en cada archivo fuente. Basta con el LICENSE y el NOTICE de la raíz, y los archivos nunca las tuvieron.
- Resolver la compatibilidad fina de cada biblioteca enlazada desde VitaSDK (freetype, vita2d, etc.). Solo se inventarían las vendorizadas en `libs/`.

## Decisions

### D1. Los cambios de contenido se hacen antes de la reescritura
Los commits de LICENSE, NOTICE, README, `.gitignore`, `config.yaml` y el retiro de `.claude/` se hacen en `vitasdk-legacy`, antes de la reescritura. Así la reescritura queda como un paso mecánico sobre un árbol ya terminado, y los commits nuevos no llevan trailers (la atribución ya está desactivada en `~/.claude/settings.json`).
*Alternativa descartada:* hacerlos después, en `main`. Funciona igual, pero deja el repo nuevo publicado un tiempo con una licencia incorrecta.

### D2. La reescritura se hace con `git filter-repo` sobre un clon nuevo
El clon se crea en un directorio temporal con `git clone --no-local` desde el repo local, trayendo solo `vitasdk-legacy` y los tags `v1.00`–`v2.10`. Luego se ejecuta `filter-repo` con un `--message-callback` que elimina las líneas que cumplen `^Co-Authored-By: .*<noreply@anthropic\.com>\s*$` (sin distinguir mayúsculas) y las líneas en blanco que queden al final. La rama se renombra a `main`.
*Alternativas descartadas:* `git filter-branch`, que está deprecado, es lento y no genera un mapa de commits. `rebase` con reword, que no escala a 83 commits y arriesga reescribir contenido.
`filter-repo` además reescribe solo los hashes abreviados que aparecen **en mensajes de commit**.
*Ajuste encontrado al aplicar:* sobre el historial completo, `filter-repo` cambia **todos** los hashes, incluso con un callback que no toca los mensajes sin trailer. La causa es que `git fast-export` (Git 2.48) descarta la firma `gpgsig` de los merges que GitHub firmó en el historial de Joel16, y eso arrastra a todos los descendientes. Por eso la reescritura se limita al rango `07f364e..vitasdk-legacy` (`--refs`). Ese rango empieza justo antes de `3ea284a`, el primer commit con trailer, y ninguno de sus 90 commits está firmado. Además, el callback devuelve sin cambios los mensajes que no tienen trailer.

### D3. Los hashes de los documentos archivados se actualizan en un commit posterior, usando el mapa
`filter-repo` deja el mapa en `.git/filter-repo/commit-map`. Un script de un solo uso (que vive fuera del repo) recorre `openspec/changes/archive/**/*.md` y reemplaza solo los tokens hexadecimales **entre backticks** que sean el prefijo único de un hash viejo que cambió. El resultado va en un commit "Point archived records at the rewritten history".
*Por qué no reescribir los archivos en cada commit histórico:* los documentos viejos del historial seguirían apuntando a hashes que ya no existen de todos modos, y el efecto que importa es que el estado actual quede coherente.
*Por qué solo entre backticks y solo prefijos del mapa:* hay números como `1999776` que también son hexadecimales válidos.

### D4. Se conservan solo los tags de Joel16 que están en el historial
Se publican `v1.00`–`v2.10`. Los tags de ElevenMPV-A no se llevan, porque apuntan a commits que no existen en el repo nuevo.

### D5. La licencia es GPLv3 ("or later"), con NOTICE
- `LICENSE` pasa a ser el texto completo de GPL-3.0.
- El `NOTICE` nuevo indica que Material-Eleven es GPL-3.0-or-later. También declara que se basa en ElevenMPV, © Joel16 bajo Apache-2.0 (y que el código original sigue cubierto también por esos términos), que incluye código derivado de ElevenMPV-A, © GrapheneCt y colaboradores bajo GPL-3.0 (en `vitaaudiolib.c` y el menú de EQ), y lista los componentes de terceros con su licencia.
- El texto de Apache-2.0 se conserva en `LICENSES/Apache-2.0.txt`, porque §4a exige entregarlo junto con el código de Joel16. Las licencias de las bibliotecas de `libs/` van en `LICENSES/` después de identificar la versión y la licencia de cada una (mpg123, dr_libs, libogg, libvorbis, libopus, libxmp-lite). Si no se puede identificar alguna, queda anotado en el NOTICE en lugar de inventarse.
Inventario verificado en la tarea 1.3. `libs/` contiene solo `libopus.a`, `libopusfile.a`, `libxmp-lite.a` y los headers `dr_flac.h`, `dr_wav.h` y `opus/*`. mpg123, libFLAC, libvorbis y libogg no están vendorizadas: se enlazan desde VitaSDK.

| Componente | Versión | Licencia | Evidencia |
|---|---|---|---|
| dr_flac / dr_wav | v0.12.2 / v0.11.1 | Dominio público (Unlicense) o MIT-0 | Bloque final de cada header |
| libopus | 1.3 | BSD-3-Clause (Xiph y otros) | Cadena `libopus 1.3` en el `.a`, `COPYING` de v1.3 |
| libopusfile | sin versión en el `.a` | BSD-3-Clause (Xiph) | Cabecera de `opusfile.h` y `COPYING` upstream |
| libxmp-lite | 4.4.1 | MIT | `XMP_VERSION` en `xmp.h` y `lite/README` del tag `libxmp-4.4.1` (la libxmp completa es LGPL, la lite es MIT) |

Todas son compatibles con GPLv3. El NOTICE también nombra, sin copiar sus textos, las bibliotecas que se enlazan estáticamente desde VitaSDK: mpg123 (LGPL-2.1), libFLAC, libvorbis y libogg (BSD-3-Clause), vita2d (MIT), FreeType (FTL), libpng, libjpeg (IJG), zlib y bzip2.

*Por qué "or later":* es lo que declara ElevenMPV-A, verificado en `master` (tarea 1.2). La única cabecera de licencia del árbol, en `ElevenMPV-A-DE/download_enabler.cpp`, dice "either version 3 of the License, or (at your option) any later version". `ElevenMPV-A/source/audio/vitaaudiolib.cpp`, el archivo del que deriva el código portado, no tiene cabecera. En ese caso la GPLv3 §14 permite elegir cualquier versión publicada. En ningún lugar aparece una restricción a "version 3 only".

### D6. El README se reescribe como fork modificado
El título pasa a ser "Material-Eleven". El primer párrafo dice que es un fork modificado de ElevenMPV de Joel16 y resume qué cambió: el skin estilo Material You, la biblioteca, la cola y shuffle, y el audio portado de ElevenMPV-A. Tiene una sección "Licencia" y la sección "Créditos" se amplía. Con eso se cumplen Apache-2.0 §4b y GPLv3 §5a de forma visible.

### D7. `config.yaml` adapta la metodología de jeopardy a este stack
Se conserva la estructura y el espíritu de jeopardy:
- La rama `change/<nombre>` se crea desde `main` y se integra solo mediante PR con CI en verde.
- Se mantienen las secciones "Fuera de alcance" y "Notas de versión" y las tareas del tamaño de un commit.
- Se mantienen `CHANGELOG.md` y los tags anotados sobre el merge.

Lo que cambia:
- `context` describe C, CMake y VitaSDK en WSL, las pruebas en consola y el `.vpk` como producto.
- La validación es "compila el `.vpk` sin warnings nuevos", en lugar de lint, typecheck y Vitest. Los tests en PC se agregan cuando existan.
- La regla de diseño de separar la lógica pura de React pasa a ser "mantener la lógica (cola, biblioteca) sin llamadas a vita2d/SCE".
- `npm version` se reemplaza por subir `VITA_VERSION`.
- La pieza de despliegue es adjuntar el `.vpk` a un GitHub Release del tag.
- La regla "Si el CI aún no existe…" se conserva, porque aplica a `add-ci-build`.
- La línea de idioma queda como español.
- Este mismo cambio es la excepción a la regla de ramas. Es el bootstrap que crea `main`, así que se ejecuta en `vitasdk-legacy`.

### D8. Versionado: SemVer en tags y `APP_VER` = `%02d.%d%d`
El tag `vX.Y.Z` se corresponde con `VITA_VERSION` `XX.YZ`, lo que exige que Y y Z valgan como máximo 9. La primera versión de Material-Eleven es **v3.0.0**, o `03.00`, un salto major sobre el `02.10` actual por la licencia, el repo y el nombre nuevos. `CHANGELOG.md` nace con esa entrada.
*Alternativa:* reiniciar en v1.0.0. Se descarta porque bajaría el `APP_VER` respecto de una instalación existente con el mismo Title ID.

### D9. `.claude/` deja de versionarse. `AGENTS.md` se queda
Se elimina la excepción `!.claude/CLAUDE.md`, la regla pasa a ser `.claude/` y se ejecuta `git rm --cached .claude/CLAUDE.md`. `AGENTS.md` sigue versionado como documento neutral para agentes, y `.codex/hooks.json` también, sin cambios.

## Risks / Trade-offs

- [La reescritura toca contenido, no solo mensajes] → Verificar que `git diff <vitasdk-legacy> main` salga vacío **antes** del commit de D3, y que `git log main | grep -ci anthropic` dé 0.
- [El fork viejo sigue publicando el historial con trailers] → Borrarlo al final (Migration Plan, paso 8), solo cuando el repo nuevo esté verificado y el usuario lo confirme explícitamente.
- [Borrar el fork es irreversible y rompe los enlaces viejos] → Antes de borrarlo, guardar un `git bundle` de todas las ramas y tags del repo local en un lugar fuera del repo.
- [Un reemplazo de hash equivocado en un documento archivado] → El reemplazo se limita a los tokens entre backticks que sean prefijo único en el mapa. El commit de D3 se revisa con `git diff --word-diff`.
- [La licencia de alguna biblioteca de `libs/` resulta incompatible con GPLv3] → Queda registrado como hallazgo y se abre un cambio aparte. No bloquea la migración, porque el problema existiría igual bajo Apache.
- [`gh` sin el scope `delete_repo`] → Se pide `gh auth refresh -s delete_repo` justo antes del paso 8.

## Migration Plan

Todos los pasos que modifican GitHub o descartan historial los ejecuta el agente solo con confirmación explícita del usuario, paso por paso.

1. **Preparar el árbol:** resolver los cambios sin commit (`.claude/CLAUDE.md`, `AGENTS.md`) y hacer los commits de D5, D6, D7, D8 y D9 en `vitasdk-legacy`. Verificar que compila el `.vpk`.
2. **Respaldo:** `git bundle create <fuera-del-repo>/ElevenMPV-pre-migracion.bundle --all`.
3. **Reescritura:** clon nuevo en el scratchpad, `filter-repo` (D2) y la rama renombrada a `main`. Verificar el diff vacío y el conteo de "anthropic" en cero.
4. **Hashes archivados:** script de D3 y commit.
5. **Publicar:** `gh repo create Perroabuelo/Material-Eleven --public --source <clon> --push`, más el push de `v1.00`–`v2.10`. Revisar que la rama por defecto sea `main`.
6. **Verificar en limpio:** clonar desde GitHub y compilar el `.vpk`.
7. **Repuntar el repo local:** `origin` pasa a apuntar a Material-Eleven y `upstream` a `joel16/ElevenMPV`. `main` sigue a `origin/main` y se borran las ramas locales `master` y `vitasdk-legacy`.
8. **Retirar el fork:** `gh repo delete Perroabuelo/ElevenMPV`, con confirmación explícita.
9. **Etiquetar v3.0.0:** tag anotado sobre `main` y publicarlo. Si `add-ci-build` todavía no existe, el `.vpk` se adjunta a mano al Release.
10. **Renombrar el directorio (D10):** con la sesión cerrada, renombrar el directorio del repo y la carpeta de proyecto de Claude Code, abrir una sesión nueva en `Material-Eleven` y reindexar.

**Rollback:** hasta el paso 5 no se ha publicado nada y basta con descartar el clon. Entre el 5 y el 8, se borra el repo nuevo y todo sigue como estaba. Después del 8, se recupera desde el bundle del paso 2.

### D10. El directorio local se renombra a `Material-Eleven`, al final y fuera de la sesión
`Proyectos/ElevenMPV` pasa a llamarse `Proyectos/Material-Eleven`. Se hace como último paso porque la sesión del agente corre dentro de ese directorio. Además, Claude Code asocia la memoria y el historial del proyecto a la ruta (`~/.claude/projects/C--Users-fotos-Documents-Proyectos-ElevenMPV/`), así que esa carpeta se renombra igual para no perder la memoria. `.repowise/` vive dentro del repo y se mueve con él. Luego se reindexa Repowise.
