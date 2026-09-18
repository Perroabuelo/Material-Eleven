> **Regla de entrega, válida para todo el change: los commits son siempre por partes.**
> Cada grupo numerado de abajo es una etapa y se entrega en sus propios commits, nunca
> agrupando varias etapas en un commit único, y dentro de una etapa se prefiere un commit
> por unidad coherente de trabajo antes que un commit grande al final. Cada etapa deja el
> árbol compilable y funcional, y **no se inicia la etapa siguiente hasta que la actual pasó
> su prueba en hardware**. Así un fallo queda atribuible a una etapa concreta por bisección,
> que es como se localizaron los dos crasheos de `vectorize-ui-controls`.

> **Protocolo de prueba en hardware (PPH).** Varias tareas lo citan por nombre. Consiste en:
> arranque en frío; cambio repetido de track alternando tracks con y sin carátula embebida;
> alternancia rápida entre Now Playing, Folders y Settings; navegación por carpetas profundas
> y vuelta atrás; y diez minutos de reproducción continua. Se considera superado si no se
> genera ningún volcado `psp2core` y la interfaz sigue respondiendo.

## 1. Instrumentación y defensas (ninguna etapa posterior empieza sin esto)

- [x] 1.1 Agregar símbolos de depuración a los flags de compilación y verificar que el binario resultante permite a `vita-parse-core` resolver nombres de función y números de línea sobre un volcado de prueba
- [x] 1.2 Establecer el punto único de destrucción de recursos gráficos, que sincroniza el dibujo pendiente y después libera, y migrar a él todas las liberaciones existentes de textura y de tipografía; verificar por inspección que no queda ninguna liberación directa fuera de ese punto y que ninguna sincronización vive dentro de una rama condicional
- [x] 1.3 Validar el resultado de toda carga de imagen y de tipografía antes de usarla, eliminando la dereferencia sin comprobación que hoy ocurre al fijar los filtros de una textura recién cargada; verificar forzando el fallo de una carga que la aplicación sigue en pie
- [x] 1.4 Añadir el contador de agotamiento del pool de vértices y la superposición de depuración conmutable con memoria gráfica libre y marca de agua del pool; verificar que la superposición muestra valores plausibles y que puede activarse y desactivarse en ejecución
- [ ] 1.5 Ejecutar el PPH sobre esta etapa y registrar la memoria gráfica libre de referencia, que servirá de línea base para las etapas siguientes
- [ ] 1.6 Confirmar que la interfaz se ve exactamente igual que antes de la etapa, dado que esta etapa no debe mover ningún píxel

## 2. Nitidez del dibujo y del texto

- [x] 2.1 Reemplazar la inicialización de vita2d por la variante que acepta modo de suavizado y tamaño de pool, comprobando el retorno y degradando a inicialización simple si el modo pedido no se puede establecer; verificar que la aplicación arranca en ambos caminos forzando el fallo del modo pedido
- [ ] 2.2 Medir los fotogramas por segundo con suavizado activo en las tres pantallas y compararlos con la etapa anterior, dejando el resultado registrado; si el costo obliga a bajar el modo, ajustarlo aquí y no en etapas posteriores
- [x] 2.3 Reemplazar los dos handles de tipografía compartidos por un handle por cada tamaño realmente dibujado, con nombres que hagan imposible dibujar un tamaño con el handle de otro; verificar comparando en pantalla un mismo carácter en dos tamaños distintos, que deben verse con el mismo nivel de definición
- [ ] 2.4 Comprobar que la nitidez ya no depende del recorrido: llegar a Now Playing por dos rutas de navegación distintas y con tracks distintos, y verificar que el título se ve igual en ambos casos
- [x] 2.5 Derivar la línea base del texto de las métricas de la tipografía al tamaño dibujado en vez del recuadro de la cadena concreta; verificar que dos filas consecutivas, una con caracteres descendentes y otra sin ellos, comparten línea base, y que el tiempo transcurrido no salta verticalmente al cambiar de valor
- [ ] 2.6 Revisar la memoria gráfica libre con la superposición frente a la línea base de 1.5, ya que esta etapa suma atlas de tipografía y modo de suavizado a la vez
- [ ] 2.7 Ejecutar el PPH sobre esta etapa

## 3. Escala tipográfica, densidad y overflow

- [ ] 3.1 Consolidar los siete tokens tipográficos actuales en los cinco de la escala nueva y aplicar el factor de densidad de la pantalla; verificar que ningún texto de ninguna pantalla queda por debajo del tamaño mínimo, revisando explícitamente reloj, porcentaje de batería, badges de formato y hints de botones
- [ ] 3.2 Subir el alto de fila de la lista de carpetas y el alto de la barra de hints a los valores que la escala nueva exige, y rehacer el layout de Folders; verificar que las filas no se solapan y que el mini reproductor y la barra de hints siguen visibles
- [ ] 3.3 Rehacer el layout de Settings con la escala nueva; verificar que la lista de categorías y el área de detalle siguen mostrando lo mismo que antes, sin recortes involuntarios
- [ ] 3.4 Rehacer el layout de Now Playing con la escala nueva; verificar que carátula, título, artista, badge, barra de búsqueda, controles y vista previa de siguientes siguen cabiendo sin solaparse
- [ ] 3.5 Llevar los targets táctiles de los controles de pista anterior y siguiente y de las entradas de la barra de navegación al mínimo táctil; verificar tocando cerca de los bordes del área activa que el toque se registra
- [ ] 3.6 Implementar el recorte de texto con indicador de continuación sobre el recorte rectangular, habilitándolo y deshabilitándolo alrededor de cada dibujo acotado; verificar con un nombre de archivo largo que no invade el badge ni la columna vecina, y con un título largo que no se sale del panel
- [ ] 3.7 Recorrer las tres pantallas buscando recorte que haya quedado activo por una ruta de salida temprana, comprobando que nada desaparece después de un dibujo acotado
- [ ] 3.8 Ejecutar el PPH sobre esta etapa

## 4. Cierre de las islas rasterizadas

- [ ] 4.1 Reemplazar el indicador de batería por una forma vectorial con nivel y estado de carga; verificar que refleja los mismos niveles que la lógica actual, incluido el tramo que hoy queda inalcanzable por la escalera de porcentaje, y que se ve con la misma definición en todos ellos
- [ ] 4.2 Reemplazar los íconos de tipo de entrada de la lista por geometría vectorial; verificar en una carpeta con subcarpetas, audio reconocido y archivos no reconocidos que cada entrada muestra su ícono
- [ ] 4.3 Simplificar el dibujo del reloj para que la cadena se calcule una sola vez por fotograma, eliminando además la rama condicional cuyos dos caminos producen el mismo resultado; verificar que el ancho medido y el texto dibujado siempre coinciden
- [ ] 4.4 Revisar el contador de agotamiento del pool tras sumar la geometría de esta etapa y, si el margen quedó estrecho, ajustar el tamaño del pool en la llamada de inicialización
- [ ] 4.5 Retirar de los recursos y de la carga las imágenes y la tipografía que ya no dibuja ninguna pantalla, incluidas las que no se dibujaban antes de este change; verificar que la aplicación compila, arranca y que la memoria gráfica libre al inicio mejora respecto de la línea base
- [ ] 4.6 Corregir el comentario del código que afirma que la biblioteca no ofrece recorte, dejando constancia de que el recorte rectangular existe y que lo que falta es el recorte por forma arbitraria
- [ ] 4.7 Ejecutar el PPH sobre esta etapa

## 5. Cobertura de glifos no latinos

- [ ] 5.1 Comprobar en hardware qué escrituras cubren las tipografías del sistema de la consola, dejando registrado el resultado; si alguna de las esperadas no estuviera cubierta, acotar el alcance de esta etapa a lo que la consola sí representa antes de seguir
- [ ] 5.2 Cargar el respaldo del sistema únicamente en los tamaños donde aparece contenido del usuario, a los píxeles exactos del token y con factor de escala neutro; verificar que el texto latino dibujado con el respaldo mide lo mismo que con la tipografía propia
- [ ] 5.3 Implementar la partición de la cadena por rango de codepoint y el dibujo por tramos, avanzando la posición con el ancho que informa cada motor; verificar con un nombre que mezcla escrituras latina y no latina que la línea se dibuja completa y sin huecos entre tramos
- [ ] 5.4 Verificar con archivos cuyos nombres estén en japonés, chino y cirílico que aparecen dibujados en la lista de carpetas, en el título y en el artista, en lugar de quedar en blanco
- [ ] 5.5 Implementar la cuenta de codepoints no latinos distintos ya dibujados y la recreación del handle de respaldo al cruzar el umbral, pasando por el punto único de destrucción de 1.2 y agendada en un cambio de carpeta, nunca dentro del dibujo de un fotograma
- [ ] 5.6 Probar la recreación cruzando el umbral dos veces seguidas mientras se navega contenido no latino, verificando que los caracteres se siguen dibujando después de cada recreación y que no se genera ningún volcado; esta es la ruta más peligrosa del change y no se da por cerrada sin esta prueba
- [ ] 5.7 Ejecutar el PPH sobre esta etapa

## 6. Cierre del change

- [ ] 6.1 Recorrer los requirements de `ui/typography` escenario por escenario sobre la consola y confirmar que cada uno se cumple
- [ ] 6.2 Recorrer los requirements de `ui/rendering` escenario por escenario sobre la consola y confirmar que cada uno se cumple
- [ ] 6.3 Confirmar que los cinco specs vigentes siguen comportándose igual: el nav rail lista las mismas tres destinaciones y marca la activa, los badges de formato aparecen en las mismas extensiones, el filtro por nombre sigue acotado a la carpeta actual, los ajustes conservan su efecto y persistencia, y el acento sigue derivándose de la carátula con su reserva fija
- [ ] 6.4 Evaluar sobre la consola si los tamaños chicos necesitan un peso tipográfico adicional y, de ser así, incorporarlo como ajuste aditivo; si no hace falta, dejarlo registrado para no volver a abrirlo
- [ ] 6.5 Revisar que ninguna etapa dejó tareas de verificación sin registrar su resultado, en particular las mediciones de fotogramas por segundo, de memoria gráfica libre y de cobertura de escrituras

## Registro de verificacion

Las tareas que sólo se cierran con la consola en la mano quedan sin marcar hasta
que se ejecuten sobre hardware. Esta tabla registra qué se verificó, cómo y con
qué resultado, y es lo que la tarea 6.5 recorre al cerrar el change.

| Tarea | Estado | Cómo / resultado |
|---|---|---|
| 1.1 símbolos de depuración | verificado | `-g` junto a `-O3`. El ELF trae 18 secciones `.debug_*` y `arm-vita-eabi-addr2line -f` resuelve `UI_GpuFreeTexture` a `source/ui_gpu.c:34`. |
| 1.2 punto único de destrucción | verificado por inspección | Cero llamadas a `vita2d_free_texture` / `vita2d_free_font` fuera de `source/ui_gpu.c`. Las tres sincronizaciones del repositorio están en el primer nivel de su función, ninguna dentro de una rama. |
| 1.3 validación de cargas | verificado por inspección | `Texture_LoadImageBilinear` retorna antes de fijar filtros si el PNG no decodifica; los cuatro accesos de texto y `UI_GpuDrawTexture` toleran un recurso nulo. Falta forzar el fallo en consola. |
| 1.4 contador y superposición | implementado | Pendiente de consola: valores plausibles y conmutación en ejecución (L + R + SELECT). |
| 1.5 PPH etapa 1 + línea base de memoria | pendiente de consola | |
| 1.6 la interfaz no se movió | pendiente de consola | |
| 2.1 MSAA con degradación | implementado, **con desvío del diseño** | `vita2d_init_advanced_with_msaa` retorna `1` incondicionalmente: el desensamblado de `libvita2d.a` muestra `movs r0, #1` en sus dos salidas e ignora todo error de `sceGxm`. Comprobar el retorno, como pide `design.md`, no puede detectar nada. La degradación se decide **antes** de la llamada, según el CDRAM libre que informa `sceKernelGetFreeMemorySize`. El modo elegido queda visible en la superposición. |
| 2.2 FPS con suavizado | pendiente de consola | |
| 2.3 un handle por tamaño | implementado | 8 pares cara/tamaño en uso, un handle cada uno (~2 MB). Pendiente de consola: comparar un mismo carácter en dos tamaños. |
| 2.4 nitidez independiente del recorrido | pendiente de consola | |
| 2.5 línea base por métricas | implementado | `UI_TextBaselineY` ya no recibe la cadena: centra la extensión de la cara medida una vez sobre `"Agjy"`. El parámetro desapareció de los 27 sitios de dibujo, así que la regresión no se puede escribir. Pendiente de consola: filas con y sin descendentes. |
| 2.6 memoria gráfica vs. línea base | pendiente de consola | |
| 2.7 PPH etapa 2 | pendiente de consola | |
