## Context

Ver `proposal.md` — Why para la motivación. Lo que importa aquí es el estado actual del código:

- `source/queue.c` posee la lista de rutas y una posición, y esa posición **es** el índice dentro del arreglo de rutas. No existe ninguna noción de orden distinto del natural.
- `source/menus/menu_audioplayer.c` posee el modo (`static int state`, tres valores excluyentes) y la lógica de avance. El sorteo del barajado vive dentro de `Music_HandleNext` (`:134-146`) y se ejecuta en el instante de avanzar.
- Cuatro caminos avanzan de pista y solo uno consulta el modo. `Music_Next`/`Music_Previous` (`:194-201`) —la puerta del mini reproductor— pasan `MUSIC_STATE_NONE` escrito a mano, igual que el transporte táctil (`:349-358`) y los gatillos (`:503-509`).
- `Menu_DrawUpNext` (`:273-305`) lee `posición+1` y `posición+2` del arreglo de rutas.
- `Utils_SetMax`/`Utils_SetMin` (`source/utils.c:15-23`) no acotan: envuelven. La cola ya se recorre de forma circular.
- `Music_SeedOnce` (`:110-120`) siembra el generador una sola vez desde `cbf3783`.

Restricciones del proyecto: C sobre VITA, sin dependencias nuevas, y una cola con techo declarado de 4000 pistas cuya memoria se reserva por crecimiento (`Queue_Grow`).

## Goals / Non-Goals

**Goals:**

- Que exista **un solo lugar** que decida cuál es la próxima pista, y que ese lugar sea la cola.
- Que ninguna superficie pueda saltarse el modo vigente por olvido, como ocurre hoy con los cuatro `MUSIC_STATE_NONE`.
- Que la previsualización se sirva del mismo plan del que se sirve la reproducción, de modo que no puedan discrepar.
- Que los dos espacios de índice que aparecen con el orden no puedan confundirse entre sí.

**Non-Goals:**

- Rediseñar el transporte ni la disposición de Now Playing: cambian los estados de dos botones, no su geometría.
- Tocar el productor de carpeta (`Queue_FillFromFolder`) ni el de biblioteca. Qué entra en la cola y en qué orden natural no cambia.
- Cambiar la semántica circular de los extremos, que ya existe.

## Decisions

### 1. La permutación vive en la cola, como arreglo paralelo

`queue.c` gana un tercer arreglo de enteros, paralelo a rutas y títulos, que contiene una permutación de los índices naturales. Sin barajado es la identidad.

*Alternativa descartada:* barajar el arreglo de rutas en sitio. Destruye el orden natural, y apagar el barajado exige recuperarlo — habría que guardarlo aparte, que es el mismo coste con más sitios donde equivocarse.

*Alternativa descartada:* generar el orden al vuelo con una biyección sobre el rango, sin memoria. Elegante, pero no permite clavar la pista en curso al frente al encender el barajado, que es precisamente el comportamiento que el usuario pidió ver.

Coste: 4 bytes por pista, 16 KB con la cola en su techo. `Queue_Grow` ya hace `realloc` de dos arreglos; el tercero es la misma forma.

### 2. La posición pasa a ser un puesto del plan, y deja de ser pública

Con un orden de por medio hay dos espacios de índice: el **natural** (puesto en el arreglo de rutas) y el **slot** (puesto dentro del plan). `Queue_GetPosition`/`Queue_SetPosition` no pueden distinguir cuál devuelven, y hay una línea donde ya se cruzan hoy:

    // menu_audioplayer.c:64, dentro de Menu_InitMusic — corre en CADA carga de pista
    Queue_SetPosition(Queue_IndexOf(path));

`Queue_IndexOf` devuelve un natural. Si la posición pasa a ser un slot, esa línea planta el valor equivocado en cada avance, y el desfase es silencioso: la reproducción sigue sonando, solo que por el plan equivocado. Peor, `Queue_IndexOf` devuelve `0` cuando no encuentra la ruta, así que el fallo se disfraza de "volver al principio".

La decisión es retirar ambas funciones del header y sustituirlas por operaciones que hablan en rutas, no en índices:

    Queue_SetShuffle(SceBool on)        enciende / apaga y replanifica
    Queue_IsShuffled(void)
    Queue_Advance(SceBool forward)      mueve el slot y devuelve la ruta a abrir
    Queue_SeekToPath(const char *path)  salta a una pista elegida; devuelve SceBool
    Queue_PeekAhead(int n, ...)         la pista n puestos por delante del plan

Por dentro todo son slots; por fuera, rutas. `Menu_InitMusic` deja de tocar la posición.

*Alternativa descartada:* mantener la posición pública y documentar en qué espacio está. Depende de que cada punto de llamada lo recuerde, que es exactamente lo que falló con el modo repetido a mano en cuatro sitios.

### 3. Dónde vive cada bandera

El **barajado** vive en la cola, porque es una propiedad del orden. La **repetición** se queda en `menu_audioplayer.c`, porque es una política sobre el fin de la pista y no sobre el orden.

Esto no es cosmética: el mini reproductor avanza sin conocer el estado de la pantalla de reproducción. Mientras el barajado viviera en un `static` de esa pantalla, el mini reproductor no podría respetarlo aunque quisiera. Ese es el defecto de origen, no un síntoma suyo.

### 4. Barajado con la pista en curso clavada al frente

Al encender el barajado o al rellenar la cola, la pista en curso —o la que el usuario eligió— se coloca en el slot 0 y se aplica Fisher-Yates al resto del rango. No se vuelve a sembrar el generador: `Music_SeedOnce` ya lo hizo.

### 5. Permutación estable y recorrido circular

El plan no se regenera al agotarse; se vuelve a recorrer. *Alternativa descartada:* rebarajar en cada vuelta. Dejaría sin pasado la costura entre vueltas —"anterior" devolvería a una pista de un plan que ya no existe— y obligaría a mantener un historial aparte del plan. Con un techo de 4000 pistas, una vuelta completa no es un final que el usuario alcance en la práctica.

### 6. El rescate de pistas ilegibles camina en slots

El bucle de reintento de `Music_HandleNext` avanza hoy `selection += forward ? 1 : -1` en espacio natural. Pasa a pedir el siguiente slot. Si no, una pista rota saca la reproducción del plan sin que nada lo indique.

### 7. La previsualización es el plan, sin casos especiales

`Menu_DrawUpNext` pide a la cola las próximas pistas y dibuja lo que reciba. Como el recorrido envuelve, una cola de una sola pista anuncia esa misma pista, que es literalmente lo que va a sonar.

*Alternativa descartada:* un mensaje especial para la cola de una pista. Vuelve a poner en la pantalla una afirmación sobre el final de la cola que la cola tendría que respaldar, que es el error que este change corrige.

## Risks / Trade-offs

- **Confundir slot con índice natural en algún punto de llamada** → se retira la posición del header; la cola solo acepta y devuelve rutas. Un uso equivocado deja de compilar en vez de sonar raro.
- **`Queue_SeekToPath` sobre una ruta que no está en la cola** → devuelve `SceBool` y quien llama decide, en vez de heredar el `return 0` mudo de `Queue_IndexOf`, que hoy manda al principio de la cola sin avisar.
- **`Queue_Grow` arrastra un desajuste previo**: si el segundo `realloc` falla, el primero ya se aplicó y la capacidad no se actualiza, dejando un arreglo mayor que el otro. El tercer arreglo lo hace más alcanzable → reservar los tres antes de publicar la nueva capacidad, y no publicarla si alguno falla.
- **Colas de cero o una pista** → avanzar, retroceder y previsualizar tienen que ser inofensivos en esos dos casos; son justamente los que hoy producen el mensaje de final.
- **Nada de esto se comprueba sin escuchar** → entrega por etapas, cada una verificada en consola antes de empezar la siguiente.

## Migration Plan

Cuatro etapas, cada una en su propio commit, cada una dejando el árbol compilable y la aplicación utilizable:

1. **La cola gana el orden y la API nueva**, con el barajado siempre apagado. El comportamiento observable no cambia: es la red de seguridad, porque si algo se rompe aquí se rompe con la identidad puesta.
2. **`menu_audioplayer` pasa a la API nueva** y se retira la posición pública. El barajado empieza a respetarse en los cuatro caminos.
3. **Se parte el modo en dos banderas** independientes.
4. **La previsualización lee el plan** y se corrige el mensaje de final de cola.

Rollback: revertir el último commit deja la aplicación utilizable en la etapa anterior; el orden de las etapas está elegido para que eso siga siendo cierto en cada punto.
