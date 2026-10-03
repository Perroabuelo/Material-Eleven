## Context

El porqué está en proposal.md. Acá va el estado del código que condiciona el cómo.

**Cómo se navega hoy.** No hay una pila de pantallas. Cada pantalla tiene su propio lazo, y para cambiar de pantalla llama a la función de la otra y hace `return` cuando esa termina. Reproduciendo se abre por tres caminos:

```
  Carpetas   --confirmar--> Menu_PlayAudio(path)    --+
  Biblioteca --confirmar--> Menu_PlayQueued(path)   --+--> Menu_RunNowPlayingLoop()
  nav rail (Carpetas, Biblioteca, Ajustes)            |
             --toque-----> Menu_ShowNowPlaying()    --+
```

El botón de volver de Reproduciendo (`menu_audioplayer.c`, rama `SCE_CTRL_CANCEL` de `Menu_RunNowPlayingLoop`) llama siempre a `Menu_DisplayFiles()`.

**El estado de cada pantalla ya sobrevive a salir de ella.** La Biblioteca guarda la pestaña, el grupo abierto y la selección en variables `static` de `menu_library.c` (`view`, `inside`, `inside_name`, `selection`, `outer_selection`). Carpetas guarda la carpeta en `cwd` y la selección en el estado de `dirbrowse.c`. Por eso, llamar de nuevo a `Menu_DisplayLibrary()` o a `Menu_DisplayFiles()` ya muestra la pantalla tal como se dejó. No hace falta guardar nada más que *cuál* de las dos fue.

## Goals / Non-Goals

**Goals:**
- Que el destino del botón de volver se fije en el único momento que lo define: cuando el usuario elige una canción.
- No cambiar ninguna firma pública ni la forma en que las pantallas se llaman entre sí.

**Non-Goals:**
- Reemplazar la navegación por llamadas anidadas por una pila o una máquina de estados.

## Decisions

### 1. Reproduciendo guarda su origen en una variable `static`

En `menu_audioplayer.c` se agrega una variable `static UI_Screen playback_origin`, con valor inicial `UI_SCREEN_FOLDERS`, que es el único origen posible antes de existir la Biblioteca.

- `Menu_PlayAudio` la pone en `UI_SCREEN_FOLDERS` y `Menu_PlayQueued` en `UI_SCREEN_LIBRARY`. Las dos lo hacen solo después de que la pista abrió: si `Menu_InitMusic` falla, no se entra a Reproduciendo y el origen anterior sigue valiendo para la canción que siguiera sonando.
- `Menu_ShowNowPlaying` y `Music_HandleNext` no la tocan. Así, entrar por el nav rail o cambiar de pista no cambia adónde se vuelve.
- La rama de volver hace `Touch_Reset()` y llama a `Menu_DisplayLibrary()` si el origen es la Biblioteca, o a `Menu_DisplayFiles()` en cualquier otro caso. Después hace `return`, igual que los saltos del nav rail.

- **Alternativa descartada: pasar el origen como parámetro de `Menu_ShowNowPlaying`.** Sirve para "volver de donde entré" (la opción B, que el usuario descartó). Para "volver de donde elegí la canción", el dato nace en `Menu_PlayAudio`/`Menu_PlayQueued`, no en quien abre la pantalla.
- **Alternativa descartada: hacer `return` y dejar que el lazo que llamó siga.** Funciona cuando Reproduciendo se abrió desde la pantalla de origen, porque `Menu_PlayQueued` vuelve al lazo de la Biblioteca. Pero cuando se entra por el nav rail, quien llamó hace `return` justo después, y la app terminaría en una pantalla que depende de la historia de llamadas. Es frágil y difícil de verificar.

### 2. El origen se guarda como `UI_Screen`

Se usa el `enum` `UI_Screen` que ya existe (`ui_theme.h`) en vez de un `enum` nuevo. Ya nombra exactamente las pantallas a las que se puede volver, y es el mismo tipo que devuelve el nav rail.

## Risks / Trade-offs

- **[La pila crece con cada salto de pantalla]** → Cada cambio de pantalla, incluido el nuevo volver a la Biblioteca, es una llamada anidada que nunca se deshace. Pasa igual hoy con volver a Carpetas y con el nav rail, así que este cambio no empeora el patrón. Cada marco ocupa poco, y harían falta miles de saltos para agotar la pila del hilo principal. Queda fuera de alcance y anotado para un cambio aparte.
- **[La Biblioteca cambió desde que se eligió la canción]** → Si el usuario reescaneó o cambió de carpeta, el estado de la Biblioteca se reinició (pestaña Canciones, fila 0). Volver lleva a la Biblioteca en ese estado nuevo. Es correcto: el origen es la pantalla, y la pantalla muestra lo que hay ahora.
- **[La fila guardada ya no existe]** → La Biblioteca ya acota `selection` al número de filas en cada fotograma (`Utils_SetMax`/`Utils_SetMin`), así que una fila fuera de rango no puede quedar seleccionada.

## Migration Plan

No hay datos que migrar: el origen vive solo en RAM. Para volver atrás basta revertir el commit.

## Estrategia de pruebas

- **Compilación**: `scripts/build.sh` sin errores ni avisos con `-Wall -Werror`.
- **Pruebas en PC**: no aplican. El cambio es solo de navegación entre pantallas (llamadas a funciones de pantalla con vita2d), y no agrega lógica pura que valga aislar. La regla de mantener la lógica sin vita2d ni SCE no se ve afectada.
- **Pruebas en consola**: los scenarios de `specs/ui/nav-shell`, con la consola configurada para confirmar con Cruz y después con Círculo.

## CI

Sin cambios en el pipeline.

## Código de terceros

No se incorpora código ni assets de terceros.
