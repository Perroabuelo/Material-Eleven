Rama: `change/add-ci-build`

## Why

`openspec/config.yaml` dice que un cambio solo entra a `main` mediante un PR con CI en verde, pero el CI todavía no existe. Además, hoy la única forma de obtener un `.vpk` es compilarlo en el WSL local con `scripts/build.sh`, que no se versiona y tiene una ruta fija. Si el CI no está, la regla no se puede cumplir, nadie puede verificar un PR sin el toolchain instalado y cada release depende de adjuntar un binario a mano.

## What Changes

- Un workflow de GitHub Actions compila el `.vpk` con VitaSDK en cada PR hacia `main` y en cada push a `main`. Sube el `.vpk` como artefacto descargable del run. La compilación usa las mismas banderas que la local (`-Wall -Werror`), así que un warning nuevo pone el CI en rojo.
- Un workflow de release, disparado por un tag `vX.Y.Z`, hace cuatro cosas: compila desde ese tag, verifica que `VITA_VERSION` en `CMakeLists.txt` corresponda al tag (`X.Y.Z` pasa a ser `XX.YZ`), crea el GitHub Release con la sección de `CHANGELOG.md` de esa versión como notas y adjunta el `.vpk`.
- `main` queda protegida: no se puede hacer push directo, y un PR solo se puede fusionar cuando el check de compilación está en verde.
- Si la imagen de VitaSDK usa CMake 4, que rechaza `cmake_minimum_required(VERSION 2.8)`, se sube ese mínimo en `CMakeLists.txt`. El objetivo es que el mismo árbol compile igual en el CI y en local.

## Capabilities

### New Capabilities

Ninguna. El cambio es de infraestructura: no altera el comportamiento del reproductor, así que se declara `skip_specs: true`.

### Modified Capabilities

Ninguna.

## Fuera de alcance

- Tests unitarios en PC para la lógica pura (cola, biblioteca). Se agregarán en un cambio propio y se sumarán al mismo workflow.
- Análisis estático, formato o lint de C.
- Firmar el `.vpk` o publicarlo en tiendas homebrew como VitaDB.
- Automatizar la subida de `VITA_VERSION` o la redacción de `CHANGELOG.md`. Siguen siendo pasos de archive.
- El tag `v3.0.0` y su Release. Se crean al cerrar `migrate-to-material-eleven`, que queda como primer uso del workflow de release.

## Notas de versión

- Versión objetivo: **v3.0.0**, sin salto propio. El cambio no tiene efecto visible dentro de la app y viaja en la versión 3.0.0, que todavía no se publica.
- Cada versión publica su `.vpk` en la página de Releases de GitHub.

## Impact

- **Archivos nuevos:** `.github/workflows/build.yml` y `.github/workflows/release.yml`.
- **Archivos que pueden cambiar:** `CMakeLists.txt`, solo el mínimo de CMake y únicamente si la imagen lo exige. `CHANGELOG.md`, para agregar el bullet de esta versión.
- **GitHub:** Actions habilitado en `Perroabuelo/Material-Eleven` y una regla de protección sobre `main`. Esa regla se aplica con confirmación del usuario, porque desde ese momento también le impide a él hacer push directo.
- **Dependencia externa:** la imagen Docker `vitasdk/vitasdk` (Ubuntu 24.04 con todos los paquetes de vdpm), fijada a una serie concreta.
- **Código del reproductor:** no cambia.
