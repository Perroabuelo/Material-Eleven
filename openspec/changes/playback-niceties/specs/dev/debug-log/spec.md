## ADDED Requirements

### Requirement: Eventos del retroceso y del temporizador
Con el log prendido, además de los eventos de "Eventos que se registran", el sistema SHALL registrar:

| Categoría | Cuándo | Campos |
|---|---|---|
| `track` | al reiniciar la pista en curso porque se retrocedió con más de 3 segundos | `event=restart`, `file` |
| `sleep` | al elegir una opción del temporizador, también "Apagado" | `event=set`, `mode` |
| `sleep` | al vencer el temporizador, por tiempo o al terminar la pista | `event=expire`, `mode` |

`mode` SHALL ser `off`, `15`, `30`, `45`, `60`, `90` o `track` (el modo de fin de pista). Reiniciar una pista SHALL NOT producir una línea `queue`, porque la cola no cambia de puesto.

La línea `nowplaying` SHALL agregar al final, después de `style`, los campos `menu` (`none`, o `sleep` con el menú del temporizador abierto) y `sleep` (la opción vigente del temporizador, con los mismos valores que `mode`). Como cualquier campo de una línea de estado, un cambio de `sleep` (por ejemplo al vencer) SHALL producir una línea `nowplaying` nueva si Reproduciendo está abierta.

#### Scenario: Retroceder con más de 3 segundos
- **WHEN** con el log prendido, el usuario reproduce una pista, espera 5 segundos y pulsa L
- **THEN** el log tiene una línea `track` con `event=restart` y el `file` de esa pista, sin una línea `queue` ni una línea `track` con `event=open` entre la pulsación y esa línea

#### Scenario: Elegir y cancelar el temporizador
- **WHEN** con el log prendido, el usuario pulsa SELECT en Reproduciendo, baja una fila, pulsa el botón de confirmar, vuelve a abrir el menú, sube una fila y confirma
- **THEN** el log tiene, en este orden, líneas `nowplaying` con `menu=sleep sleep=off`, una línea `sleep` con `event=set mode=15`, una línea `nowplaying` con `menu=none sleep=15`, una con `menu=sleep sleep=15`, una línea `sleep` con `event=set mode=off` y una línea `nowplaying` con `menu=none sleep=off`

#### Scenario: Vencer al terminar la pista
- **WHEN** con el log prendido, el usuario elige "Al terminar la pista" en una carpeta de tres pistas cortas y espera a que termine la primera
- **THEN** el log tiene una línea `track` con `event=end`, una línea `sleep` con `event=expire mode=track`, una línea `track` con `event=open` y el `file` de la segunda, y una línea `nowplaying` con ese `file`, `paused=1` y `sleep=off`
