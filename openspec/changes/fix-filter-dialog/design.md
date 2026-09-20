## Context

Ver `proposal.md` — Why para la motivación y para el descarte del multisampling como causa.

Lo que importa aquí es la mecánica. El lazo que atiende el diálogo de teclado vive dentro de `Menu_PromptFilter`, en `source/menus/menu_displayfiles.c`, y sustituye por completo al lazo de fotogramas de `Menu_DisplayFiles` mientras está activo: mientras gira, ninguna otra parte de la aplicación dibuja ni lee la entrada del usuario. Esa sustitución es la que convierte un diálogo que no progresa en un bloqueo total, y es la razón por la que este change no se limita a corregir una línea.

Tres restricciones condicionan las decisiones de abajo:

- **La envoltura de composición del motor gráfico sí informa de errores.** La función de vita2d que compone el diálogo devuelve el código de error de la llamada del sistema sin alterarlo: es una llamada de cola que deja intacto el registro de retorno, comprobado por desensamblado de la biblioteca instalada en el toolchain. Existe, por tanto, una señal directa y por fotograma de "¿falló la composición?", que es exactamente el defecto que estamos corrigiendo, y no hace falta reimplementar nada contra estado interno no exportado.
- **El estado del diálogo no distingue "el usuario está pensando" de "el diálogo está muerto".** En ambos casos la consulta de estado responde lo mismo. Cualquier salida basada solo en tiempo corre el riesgo de cortarle la escritura a un usuario lento.
- **Nada de esto se puede reproducir fuera de la consola.** El defecto es de comportamiento del sistema operativo de la VITA; compilar no demuestra nada.

## Goals / Non-Goals

**Goals:**

- Que el diálogo se componga y progrese, que es la corrección del defecto.
- Que ninguna ruta de este change pueda volver a dejar la aplicación sin salida, aunque el diálogo falle por una causa distinta de la diagnosticada.
- Que el fallo, si persiste, quede en un estado diagnosticable en vez de en un cierre forzado desde el sistema.

**Non-Goals:**

- Generalizar el arreglo a una capa de diálogos reutilizable. Hoy hay un único diálogo en toda la aplicación; una abstracción aquí se escribiría sobre un solo caso de uso. El invariante queda escrito en `ui/rendering` para el siguiente que aparezca, que es donde pertenece.
- Cambiar qué busca el filtro. Sigue siendo la carpeta actual, sobre entradas ya leídas. Si más adelante llega una biblioteca indexada, la semántica de la búsqueda se replantea entonces y no ahora.
- Tocar la lógica de filtrado en `source/dirbrowse.c`, que funciona y no interviene en el bloqueo.

## Decisions

### Composición del diálogo fuera de la escena de dibujo

La actualización del diálogo se mueve detrás del cierre de la escena del fotograma y delante de la presentación del buffer. Es el orden que el motor gráfico documenta en sus propios ejemplos, y la razón es estructural: la composición del diálogo escribe en el buffer de pantalla, no en la escena, de modo que pedirla con una escena abierta es una operación inválida que falla en silencio en cada fotograma.

*Alternativa considerada y descartada:* atribuir el fallo al modo de multisampling y desactivarlo alrededor del diálogo. Descartada por la cronología — el buscador nunca funcionó, y el multisampling llegó después — y porque degradaría el suavizado que `ui/rendering` exige. Queda anotada como sospechoso siguiente en Risks.

### La salida garantizada tiene tres mecanismos, del más preciso al más tosco

La salida del lazo combina tres mecanismos, en este orden de precedencia:

1. **El código de error de la composición**: si la llamada que compone el diálogo devuelve error durante varios fotogramas consecutivos, el diálogo no se está pudiendo presentar y el lazo abandona. Es el detector principal porque es la señal exacta del defecto y no una inferencia sobre él: componer con una escena abierta es una operación inválida que falla en todos y cada uno de los fotogramas, de modo que la detección es determinista y ocurre en una fracción de segundo. Se exigen varios fotogramas y no uno solo para no abandonar por un fallo transitorio.
2. **Abandono explícito por el usuario**: una combinación de botones mantenida durante un número de fotogramas consecutivos abandona el diálogo y devuelve el control a la lista. Sigue haciendo falta aunque el mecanismo anterior funcione, porque cubre el caso en que la composición va bien pero el usuario queda atascado por cualquier otra causa. Requiere leer el pad dentro del lazo, cosa que hoy no se hace.
3. **Techo absoluto de fotogramas**: generoso, dimensionado para no interrumpir a alguien escribiendo despacio, como última red por si el diálogo queda en un estado en el que ni la composición informa ni el pad llega a la aplicación.

En los tres casos el filtro queda exactamente como estaba antes de abrir el diálogo, y el diálogo se cierra por la vía ordenada —se le pide que aborte y se espera de forma acotada a que deje de estar en ejecución— antes de volver.

*Alternativa considerada y descartada:* quedarse solo con el pad y el techo, sin mirar el código de error. Descartada al comprobar que la envoltura del motor gráfico sí propaga el error (ver Context): renunciar a la señal exacta para quedarse con dos heurísticas sería aceptar una detección más lenta y menos informativa a cambio de nada.

*Alternativa considerada y descartada:* un techo de tiempo corto y sin abandono explícito. Descartada porque le cortaría la escritura a un usuario lento, convirtiendo un bloqueo en una pérdida de texto silenciosa.

### El fondo se redibuja llamando a las mismas funciones de dibujo

En lugar de limpiar el fotograma, el lazo del diálogo dibuja la misma pantalla de carpetas que dibujaría el lazo normal. No se captura una imagen previa: se reutilizan las funciones de dibujo existentes, que ya son puras respecto de la entrada del usuario.

La consecuencia deliberada es que el mini-reproductor sigue animándose detrás del diálogo, lo cual es correcto: la reproducción no se detiene porque el usuario abra el buscador.

La regla que acompaña a esto y que conviene no perder: **mientras el diálogo está abierto se dibuja pero no se procesa entrada de pantalla**. El pad se lee solo para el abandono descrito arriba, y ninguna pulsación ni toque se encamina a los controles de la pantalla de fondo, que el usuario no puede ver que está pulsando.

### La configuración del subsistema de diálogos se hace una vez, al arrancar

Va junto a la inicialización del resto de servicios del sistema en `source/main.c`, donde la aplicación ya averigua qué botón es "aceptar" en esta consola. Hacerlo por diálogo sería repetir en cada apertura una configuración que no cambia durante la sesión.

### El botón de cancelar deshace primero el filtro y después la carpeta

Con un filtro activo, cancelar limpia el filtro y deja al usuario en la misma carpeta; solo una segunda pulsación sube a la carpeta superior. Esto **cambia un comportamiento existente** y es intencionado: el filtro es un estado superpuesto a la carpeta, y cancelar deshace lo más reciente primero, que es la convención que el usuario ya espera de un botón de volver.

Es además lo que cierra el segundo bloqueo, porque hoy la acción de cancelar solo está disponible por debajo de la raíz del dispositivo, y es justo en la raíz donde un filtro sin coincidencias deja la lista sin ninguna entrada sobre la que actuar.

*Alternativa considerada y descartada:* asignar la limpieza del filtro a un botón distinto y dejar cancelar intacto. Descartada porque añade una tecla que aprender para resolver un estado en el que el usuario ya está desorientado, y porque la leyenda de botones en pantalla tendría que crecer para anunciarla.

### Entrega por etapas

Los commits se realizan por partes, una etapa por vez, y ninguna etapa empieza sin que la anterior haya pasado su prueba en la consola. La razón es la tercera restricción del Context: nada de esto se verifica compilando, así que la bisección sobre commits pequeños es la única herramienta de diagnóstico real si algo sale mal. Es la misma regla de entrega que `close-ui-contract`, que es como se localizaron sus dos crasheos de GPU.

## Risks / Trade-offs

- **El arreglo no se puede verificar sin consola** → cada etapa es pequeña, deja el árbol funcional y se prueba en hardware antes de la siguiente; el orden de las etapas en `tasks.md` pone la salida garantizada antes que la corrección de la composición, para que la etapa que toca el diálogo ya se pruebe con red.
- **El diálogo podría seguir sin aparecer tras corregir el orden** → el siguiente sospechoso es el modo de multisampling establecido al arrancar. La salida garantizada convierte ese escenario en algo diagnosticable: el usuario vuelve a la lista y puede informar, en lugar de perder la sesión. La prueba a ejecutar en ese caso es arrancar forzando la degradación a sin multisampling y repetir la apertura del buscador.
- **El abandono por combinación de botones podría colisionar con la escritura** → la combinación se elige entre las que el teclado del sistema no usa, y exige mantenerse varios fotogramas, de modo que una pulsación accidental no la dispare.
- **El techo absoluto podría cortar a un usuario muy lento** → se dimensiona con holgura y es la última red, no el mecanismo principal; el camino previsto para abandonar es el explícito.
- **Cambiar el significado de cancelar puede sorprender a quien ya usa la aplicación** → el cambio solo es observable cuando hay un filtro activo, que es exactamente el estado en el que hoy el botón no ayuda; la leyenda de botones en pantalla refleja la acción vigente.
