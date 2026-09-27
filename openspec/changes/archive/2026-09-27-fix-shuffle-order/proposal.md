## Why

El shuffle solo se gatilla cuando la pista termina sola: ningún salto manual lo respeta —ni los botones táctiles, ni los gatillos L/R, ni el mini reproductor—, y la lista "A CONTINUACIÓN" anuncia pistas que no van a sonar. Las dos cosas son el mismo defecto: hoy el shuffle no es un orden, es un dado que se tira en el instante de avanzar (`rand()` dentro de `Music_HandleNext`, `source/menus/menu_audioplayer.c:134-146`), de modo que no existe una "siguiente pista" que respetar ni que anunciar hasta que ya es tarde.

## What Changes

- **El shuffle pasa a ser un orden, no un sorteo.** La cola gana una permutación de sus índices, paralela al arreglo de rutas. Avanzar, retroceder y previsualizar recorren esa permutación; apagar el shuffle la devuelve a la identidad. Es el cambio que habilita todo lo demás de este change: mientras el shuffle sea un dado, no hay nada que anunciar ni que respetar.
- **La permutación es estable.** Se baraja al encender el shuffle y al rellenar la cola, se recorre entera, y al agotarse vuelve a empezar la misma. Lo que "A CONTINUACIÓN" anuncia es lo que sonará, y "anterior" camina hacia atrás por la historia real del plan. Rebarajar en cada vuelta queda descartado explícitamente: dejaría sin pasado la costura entre vueltas, y una vuelta entera a una cola de miles de pistas no es un final que el usuario alcance.
- **Los cuatro caminos que avanzan de pista dejan de mentir.** Hoy el botón táctil, los gatillos y `Music_Next`/`Music_Previous` —la puerta por la que entra el mini reproductor— pasan `MUSIC_STATE_NONE` escrito a mano, y solo el fin natural de la pista consulta el estado. Pasan todos a avanzar por el mismo orden, sea cual sea.
- **La previsualización lee el plan, no los vecinos.** `Menu_DrawUpNext` toma hoy `posición+1` y `posición+2` del arreglo de rutas, que con shuffle encendido es sistemáticamente falso. Pasa a pedirle a la cola las próximas pistas del orden vigente.
- **El shuffle y la repetición dejan de ser excluyentes.** Hoy comparten un único `int state` de tres valores, así que encender uno apaga el otro sin avisar, aunque en pantalla se dibujen dos botones independientes. Pasan a ser dos banderas: el shuffle gobierna el orden del recorrido, la repetición gobierna qué ocurre cuando la pista termina. Con las dos encendidas, la pista queda en bucle y "siguiente" sigue avanzando por el plan barajado, que hoy es inalcanzable.
- **La cola deja de exponer su posición como índice del arreglo.** Con un orden de por medio hay dos espacios de índice —el natural y el puesto dentro del plan—, y `Queue_GetPosition`/`Queue_SetPosition` no distinguen cuál devuelven. Se retiran de la interfaz pública en favor de operaciones que hablan en rutas, para que el desfase entre ambos espacios no pueda reaparecer por otra puerta. Es la corrección estructural del change: sin ella, `Menu_InitMusic` planta un índice natural donde ahora se espera un puesto del plan.
- **Se corrige el mensaje de final de cola.** "No hay mas pistas en esta carpeta" es falso hoy mismo: `Utils_SetMax` (`source/utils.c:15`) no acota, envuelve, así que la cola ya se repite entera para siempre. La previsualización es justamente lo que este change arregla, y no puede quedarse diciendo que la reproducción termina donde no termina.
- **Entrega por partes.** Los commits van por etapas, nunca agrupando el change en uno solo, y cada etapa deja el árbol compilable y verificado en consola. Es la misma regla de entrega de `add-music-library` y `fix-filter-dialog`, y aquí importa porque el orden de reproducción solo se comprueba de verdad escuchándolo.

**Fuera de alcance, decidido explícitamente:** persistir el shuffle y la repetición entre arranques (hoy nacen apagados en cada sesión y así se quedan); rebarajar en cada vuelta y el historial separado del plan que eso exigiría; reordenar, eliminar o editar la cola, que `ui/now-playing` prohíbe expresamente mientras no exista esa pantalla; y la visualización de letras, explorada junto a esto y apartada a un change propio para que la corrección del shuffle no quede detenida detrás de un diseño de pantalla sin resolver.

## Capabilities

### New Capabilities

- `playback/queue`: qué es la cola de reproducción y en qué orden se recorre. Hoy `source/queue.c` existe desde `a180578` sin ninguna capability que lo describa, y su comportamiento solo aparece de refilón dentro de `ui/now-playing` —un spec de una pantalla que no gobierna al mini reproductor ni a la biblioteca, que también avanzan por la cola—. Cubre: quién la llena y con qué orden natural, el orden barajado y su estabilidad, qué significa avanzar y retroceder en cada modo, qué ocurre al llegar a los extremos, cómo se resuelve saltar a una pista concreta, y qué pasa cuando una pista de la cola no se puede abrir.

### Modified Capabilities

- `ui/now-playing`: el requirement de los controles de transporte fija hoy que el shuffle y la repetición conservan "su efecto actual", lo que incluye tanto que solo se apliquen al terminar la pista como que sean mutuamente excluyentes. Pasa a exigir que sean dos controles independientes y que cualquier avance de pista respete el modo vigente, venga del botón, del gatillo o del mini reproductor. El requirement de previsualización de próximas pistas pasa a exigir que lo mostrado sea el orden en que realmente sonarán, y que el mensaje de cierre no afirme un final que la cola no tiene.

## Impact

- `include/queue.h`, `source/queue.c`: la permutación, su generación y las operaciones que sustituyen a la posición pública. Crece un tercer arreglo paralelo a rutas y títulos, con el mismo `realloc` de `Queue_Grow`: 16 KB con la cola en su techo declarado de 4000 pistas.
- `source/menus/menu_audioplayer.c`: `Music_HandleNext` deja de sortear y pasa a solo avanzar y abrir; `Music_Next`/`Music_Previous` dejan de forzar el modo; los cuatro puntos de avance y el bloque de fin de pista se reescriben; `Menu_DrawUpNext` pasa a leer el plan; `Menu_InitMusic` deja de fijar la posición; el `int state` de tres valores se parte en dos banderas.
- `source/mini_player.c`: sin cambios. Sigue llamando a `Music_Previous`/`Music_Next`, que es precisamente lo que pasa a comportarse bien.
- `source/menus/menu_library.c`: el `Queue_SetPosition` de la línea 371 queda cubierto por el salto a ruta y deja de ser necesario.
- Sin dependencias nuevas. `Music_SeedOnce` ya siembra el generador una sola vez desde `cbf3783`, que es justo lo que necesita un barajado de Fisher-Yates.
