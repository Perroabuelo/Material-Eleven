## Context

Ver `proposal.md` - Why para la motivación. Lo que sigue es el estado técnico verificado que da forma al enfoque.

**Verificado contra el toolchain instalado** (`vita2d.h` y `libvita2d.a` del VITASDK en WSL, más desensamblado con `arm-vita-eabi-objdump`):

- `vita2d_init_advanced_with_msaa(unsigned int temp_pool_size, SceGxmMultisampleMode msaa)` existe. Hoy `source/main.c` llama a `vita2d_init()` a secas, que no activa suavizado alguno, y descarta su valor de retorno.
- **El atlas de glifos de `vita2d_font` se indexa solo por índice de glifo, sin el tamaño.** En `generic_font_draw_text`, la clave que se pasa a `texture_atlas_get` es el resultado de `FTC_CMapCache_Lookup` y nada más; `texture_atlas_insert` la reenvía intacta a `int_htab_insert`. Ante un acierto, la función calcula el cociente entre el tamaño pedido y el tamaño con que el glifo quedó cacheado, y lo usa como factor de escala de `vita2d_draw_texture_tint_part_scale`. Es decir: **cada glifo se rasteriza una sola vez, al tamaño en que se dibujó por primera vez, y todo otro tamaño es un reescalado bilineal de ese único bitmap.** La aplicación usa hoy 7 tamaños sobre 2 handles, así que la mayoría del texto se dibuja reescalado, y qué glifo queda nítido depende del orden de navegación.
- `vita2d_pvf` tiene la misma estructura de atlas, y además fija el tamaño de carácter en la carga (`scePvfSetCharSize` solo aparece dentro de las funciones de carga): su API de dibujo recibe un factor de escala, no un tamaño en píxeles.
- **El módulo de atlas no tiene desalojo**: expone `create`, `insert`, `get`, `exists` y `free`, nada más. Cuando la hoja de 512x512 se llena, `texture_atlas_insert` falla y `generic_font_draw_text` salta al carácter siguiente: **el carácter no se dibuja, sin error ni marca visible**.
- Cada handle de fuente crea su propio atlas de 512x512 a 1 byte por texel, es decir **256 KB por handle**.
- `vita2d_set_clip_rectangle`, `vita2d_enable_clipping` y `vita2d_disable_clipping` existen. El comentario de `source/menus/menu_audioplayer.c` que afirma que vita2d no ofrece recorte es incorrecto en cuanto al recorte rectangular; sigue siendo cierto que no hay stencil ni recorte por forma arbitraria.
- `vita2d_load_system_pvf(int numFonts, const vita2d_system_pvf_config *configs)` acepta un predicado `in_font_group(unsigned int c)` por fuente, y `get_font_for_character` elige la fuente por codepoint. Es un mecanismo de respaldo multi-fuente incorporado, pero solo entre fuentes del sistema: mezclar la tipografía propia con la del sistema hay que resolverlo en la aplicación.

**Aritmética de la pantalla.** La consola tiene 960x544 en 5 pulgadas, es decir unos 220.7 ppp, o sea que un píxel mide 0.115 mm y una unidad independiente de densidad del lenguaje de diseño de referencia equivale a 1.379 px. Los mockups de `templates/` están hechos a 960x544 y usan tamaños de 10 a 21 px, pero se revisaron en un navegador de escritorio, donde ese lienzo ocupa unos 25 cm de ancho en vez de 11. El código copió esos valores tal cual, de modo que los 7 tokens actuales (11, 12, 13, 14, 16, 19, 22 px) equivalen físicamente a 8, 8.7, 9.4, 10, 11.6, 13.8 y 16 unidades: **toda la escala está por debajo, y los dos tamaños más chicos quedan bajo el mínimo de etiqueta del propio lenguaje que se imita**. El mismo error de origen afecta a los targets táctiles: 44 px son 5.1 mm y 40 px son 4.6 mm, contra un mínimo de 7.6 mm.

**Historial de fallos.** `vectorize-ui-controls` produjo dos crasheos de GPU en hardware, ambos de la misma clase: la CPU liberó o reutilizó memoria que la GPU todavía estaba leyendo. En `ac8f53d` los vértices vivían en el stack, que ni siquiera está mapeado para la GPU. En `f3d908e` la sincronización existía pero estaba dentro de la rama que libera la carátula, así que solo corría cuando el track saliente tenía portada; su propio mensaje señala que la carrera era anterior y que la vectorización solo ensanchó la ventana hasta hacerla visible. Hoy queda **una sola llamada de sincronización en todo el repositorio**: las 24 liberaciones de textura de `source/textures.c` y las 3 de fuente corren sin ella, y `Texture_LoadImageBilinear` dereferencia la textura recién cargada sin comprobar que no sea nula.

## Goals / Non-Goals

**Goals:**

- Que la nitidez del texto sea una propiedad del código y no del orden en que el usuario navegó.
- Que la escala tipográfica y los targets táctiles se deriven del tamaño físico de la pantalla, no de números copiados de un mockup.
- Que las invariantes de ciclo de vida de recursos GPU se cumplan por construcción y no por disciplina, dado que la disciplina ya falló una vez con la sincronización mal ubicada.
- Que cada etapa quede verificable por separado en hardware, para que un fallo sea atribuible por bisección.

**Non-Goals:**

- No se corrige la biblioteca `vita2d`: se trabaja alrededor de su comportamiento, no se vendoriza ni se parchea (ver Decisions).
- No se persigue un recorte por forma arbitraria ni esquinas redondeadas sobre la carátula: el recorte disponible es rectangular.
- No se introduce ninguna pantalla, transición ni gesto nuevo; el alcance excluido está listado en `proposal.md` - Impact.
- No se rediseña el muestreo de color de carátula ni el ajuste de legibilidad del acento: `ui/dynamic-accent` queda como está.

## Decisions

**Un handle de fuente por cada tamaño dibujado, en vez de arreglar la biblioteca.** Dado que el atlas ignora el tamaño en su clave, la única forma de obtener un bitmap por tamaño con la biblioteca instalada es darle a cada tamaño su propio atlas, es decir su propio handle. El nombre del handle pasa a ser el contrato: si el único handle disponible para un tamaño es el de ese tamaño, la regresión no se puede escribir. Se evaluó y se descartó **vendorizar y parchear `vita2d_font`** — sería la solución de raíz, arreglaría la clave del atlas y permitiría un handle único para todos los tamaños, y además habilitaría un desalojo LRU que resolvería el techo del atlas. Se descarta porque el toolchain instala únicamente `vita2d.h`: los encabezados internos del subsistema de atlas no están, de modo que habría que traer el subsistema completo desde el repositorio de vita2d y resolver la colisión de símbolos con la biblioteca ya enlazada, pasando a mantener un fork de una biblioteca del toolchain. Es desproporcionado frente a un defecto que se resuelve con contabilidad de handles. Queda anotado aquí como el camino documentado si el problema reaparece por otra vía.

**La escala baja de 7 tokens a 5.** Cada token es un atlas más que poblar, y siete escalones de jerarquía es más de lo que 960x544 puede expresar. Los 7 actuales se consolidan en 5 aplicando el factor de densidad: badge y mono chico a 15, etiqueta y hint a 17, cuerpo a 19, título a 22, display a 30. La familia monoespaciada no necesita los cinco — solo aparece en badges, duraciones, tamaños de archivo y reloj — así que se cargan solo los tamaños que se usan de verdad. Con del orden de 7 a 9 handles el costo de memoria gráfica ronda los 2 MB, despreciable frente a lo que la consola ofrece.

**El suavizado se pide en la inicialización y se degrada si no entra.** `vita2d_init()` pasa a `vita2d_init_advanced_with_msaa(...)`, que además permite fijar explícitamente el tamaño del pool de vértices por fotograma — hoy se usa el valor por defecto de 1 MB, y la etapa que vectoriza batería e íconos de fila sube la geometría por fotograma. Es la misma llamada, así que ambas cosas se ajustan juntas. El retorno se comprueba: si el modo pedido no se puede establecer, se reintenta sin suavizado antes de rendirse, de acuerdo con el requirement de degradación definida.

**La densidad se paga en filas, no en tamaño de letra.** Subir el cuerpo a 19 y el subtítulo a 17 obliga a llevar el alto de fila de 50 a unos 64, con lo que la lista de carpetas pasa de 7 a 5 entradas visibles. Se evaluó reducir el tamaño para conservar 7 filas y se descarta: es exactamente el compromiso que produjo el defecto original. Se evaluó también eliminar el subtítulo de tamaño de archivo para recuperar una fila, y se deja como ajuste disponible durante el relayout, no como decisión previa. Now Playing absorbe la escala nueva sin perder contenido porque su alto está dominado por la carátula.

**El overflow se resuelve por recorte rectangular, no por medición previa.** Como el recorte existe en la biblioteca, el texto se dibuja dentro de un rectángulo activo y se recorta ahí, con un indicador de continuación. Se prefiere sobre medir y cortar la cadena carácter a carácter porque no requiere medir repetidamente en cada fotograma. El recorte se habilita y se deshabilita alrededor de cada dibujo acotado, ya que dejarlo activo por descuido haría desaparecer lo que se dibuje después.

**Respaldo no latino: partición por rango de codepoint en la aplicación.** La cadena se parte en tramos según si el codepoint cae en los rangos que cubre la tipografía propia, y cada tramo se dibuja con su motor, avanzando la posición con el ancho que informa ese motor. El respaldo del sistema se carga únicamente en los tamaños donde aparece contenido del usuario — nombres de archivo, título y artista — y no en los de chrome, que es texto propio de la aplicación y siempre latino. Como el tamaño del respaldo se fija al cargar, se carga a los píxeles exactos del token y se dibuja siempre con factor de escala neutro, de modo que la regla de un handle por tamaño aplica igual a ambos motores.

**El techo del atlas se maneja recreando el handle de respaldo, y esa recreación es la ruta más peligrosa del change.** Como no hay desalojo, un alfabeto de miles de signos llena la hoja: a los tamaños en juego caben del orden de seiscientos glifos de ancho completo, contra los miles que una biblioteca en japonés o chino puede exhibir a lo largo de una sesión. La aplicación lleva la cuenta de codepoints no latinos distintos ya dibujados y, al cruzar un umbral conservador, destruye y recrea el handle de respaldo para obtener una hoja limpia. Se evaluó aceptar el techo y documentarlo, y se descarta porque contradice el requirement de cobertura sostenida durante la sesión. **La recreación libera una textura de GPU en mitad de la sesión, que es exactamente lo que provocó el segundo crasheo**, así que se realiza a través del punto único de destrucción descrito abajo y se agenda en un cambio de carpeta, nunca dentro del dibujo de un fotograma, aprovechando que ahí ya hay una pausa por lectura de disco.

**Las invariantes de GPU se hacen cumplir por construcción.** La lección de los dos crasheos no es que faltara la sincronización, sino que estaba en el lugar equivocado: la disciplina no alcanza. Se establecen cuatro puntos únicos de paso:

| Invariante | Cómo se hace cumplir |
|---|---|
| Los vértices que se envían a dibujar salen siempre del pool del fotograma | El ayudante que ya se creó tras el primer crasheo sigue siendo el único origen; enviar geometría por fuera de él queda prohibido |
| Toda destrucción de recurso gráfico sincroniza antes, sin condiciones | Un único punto de destrucción que sincroniza y después libera; nadie llama a la liberación directa, y la sincronización nunca vive dentro de una rama |
| Toda inicialización y carga valida su resultado y degrada | El suavizado cae a modo simple, una tipografía que no carga no se dibuja, y una imagen que no carga no se dereferencia |
| El agotamiento del pool es observable | Un contador expuesto en la superposición de depuración, en vez de formas que desaparecen en silencio |

**Instrumentación antes que cambios visuales.** La primera etapa no mueve un píxel: instala los cuatro puntos anteriores, agrega símbolos de depuración a la compilación —que con optimización activa no alteran el código generado, solo permiten que los volcados `psp2core` resuelvan números de línea— y añade una superposición conmutable con memoria gráfica libre, marca de agua del pool y el contador de agotamiento. Es la red antes del salto, y desactiva de paso los defectos latentes ya identificados.

**La entrega es siempre por partes.** La implementación se entrega en commits separados por etapa, nunca en un commit único, tal como se hizo en `vectorize-ui-controls`. Cada etapa deja el árbol compilable y funcional y se prueba en hardware antes de iniciar la siguiente. Esto cumple dos funciones: permite mergear el trabajo de nitidez sin esperar al relayout, y convierte un crasheo en algo atribuible a una etapa concreta por bisección, que es como se localizaron los dos anteriores.

## Risks / Trade-offs

- [La memoria gráfica sube por dos vías a la vez: los atlas de fuente y el modo de suavizado] → Es la única superficie donde dos decisiones compiten por el mismo recurso. Se mide la memoria libre con la superposición de depuración al cerrar la primera etapa, antes de que el relayout dependa de ella. Si no alcanza, la palanca es bajar el modo de suavizado, no recortar tamaños tipográficos.
- [Recrear el handle de respaldo es la misma operación que causó el segundo crasheo] → Pasa por el punto único de destrucción, se agenda en un cambio de carpeta y se prueba con un protocolo propio: cruzar el umbral dos veces seguidas navegando contenido no latino.
- [Vectorizar batería e íconos de fila sube la geometría por fotograma, que es el mecanismo que ensanchó la ventana del segundo crasheo] → El tamaño del pool se fija explícitamente en la misma llamada de inicialización, y el contador de agotamiento hace visible el margen restante antes de que se note como elementos faltantes.
- [La cobertura real de las tipografías del firmware no pudo verificarse sin consola] → Se comprueba en hardware como parte de la etapa de respaldo, con nombres en japonés, chino y cirílico. Si alguna escritura no estuviera cubierta, el requirement afectado es el de cobertura y se acota a lo que la consola sí representa; no cambia el enfoque ni el resto de las etapas.
- [El costo del suavizado en fotogramas por segundo tampoco pudo medirse sin consola] → La primera etapa incluye una medición, aunque sea burda, como criterio de cierre. La arquitectura por tiles de la consola resuelve el multimuestreo en memoria de tile, así que el costo esperado es bajo, pero es expectativa, no medición.
- [La tipografía propia se distribuye hoy en un solo peso, y a los tamaños chicos los trazos finos pueden seguir viéndose débiles aun al tamaño correcto] → Se evalúa en hardware una vez aplicada la escala nueva; incorporar un peso adicional es aditivo y no altera ninguna decisión de este diseño.
- [Perder dos filas en la lista de carpetas es una pérdida real de densidad] → Aceptado explícitamente por el usuario como el costo de la legibilidad. El ajuste disponible, si molesta en uso, es retirar el subtítulo de tamaño de archivo, que es el dato de menor valor de la fila.
- [Dejar el recorte activo por una ruta de salida temprana haría desaparecer lo que se dibuje después] → No es un fallo sino un defecto visual, y se evita habilitando y deshabilitando el recorte alrededor de cada dibujo acotado en el mismo ámbito.

## Migration Plan

No hay datos persistidos ni formatos que migrar: el cambio es de presentación. La configuración del usuario y el directorio recordado no se tocan.

La secuencia de despliegue es la de etapas de `tasks.md`, cada una en sus propios commits y con su prueba en hardware antes de la siguiente. El protocolo de prueba es el mismo para todas: arranque en frío; cambio repetido de track alternando tracks con y sin carátula, que es el caso exacto del segundo crasheo; alternancia rápida entre las tres pantallas; navegación por carpetas profundas; y diez minutos de reproducción continua. La etapa de respaldo no latino agrega el suyo propio.

La reversión es por etapa: como cada una deja el árbol compilable, revertir sus commits devuelve al estado funcional anterior sin arrastrar a las demás. La primera etapa, que es solo instrumentación, está pensada para permanecer aunque se revierta cualquier etapa posterior.

## Open Questions

Ninguna que afecte a los specs, al enfoque o al desglose de tareas. Los tres puntos que no pudieron cerrarse en este entorno —cobertura real de las tipografías del firmware, costo del suavizado en fotogramas por segundo y necesidad de un peso tipográfico adicional— requieren la consola y están registrados como tareas de verificación en `tasks.md` y como riesgos arriba, cada uno con su palanca de ajuste. Ninguno cambia qué se construye.
