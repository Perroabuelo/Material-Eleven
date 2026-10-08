# Rendering Specification

## Purpose

Fija cómo se dibujan las formas, los íconos y los controles de la interfaz, qué tamaño deben tener para poder tocarse, y cómo se manejan los recursos gráficos durante la sesión, para que la interfaz se vea nítida y no vuelva a caerse por la clase de fallo de GPU que ya se observó dos veces en hardware.

## Requirements

### Requirement: Bordes suavizados en las formas vectoriales
El sistema SHALL dibujar las formas vectoriales de la interfaz con suavizado de bordes, de modo que las diagonales y las curvas no presenten escalonado visible a la distancia normal de uso de la consola.

#### Scenario: Diagonales de los controles de transporte
- **WHEN** el usuario mira los controles de reproducción, que incluyen triángulos y trazos en diagonal
- **THEN** sus bordes inclinados se ven continuos, sin dientes de sierra

#### Scenario: Curvas del chrome
- **WHEN** el usuario mira las esquinas redondeadas de las tarjetas, los anillos y los controles circulares
- **THEN** sus curvas se ven continuas, sin escalones perceptibles

### Requirement: El chrome de la interfaz se dibuja como geometría, no como imagen
El sistema SHALL dibujar como geometría vectorial todo elemento de chrome de la interfaz —controles, íconos e indicadores de estado, incluidos el indicador de batería y los íconos de tipo de entrada de la lista— y SHALL reservar las imágenes rasterizadas para contenido real del usuario, como la carátula del track.

#### Scenario: Indicador de batería en cualquier nivel
- **WHEN** la carga de la batería cambia entre sus distintos niveles, con y sin carga conectada
- **THEN** el indicador se dibuja con la misma definición en todos los niveles, y refleja el estado de carga

#### Scenario: Íconos de las entradas de la lista
- **WHEN** el usuario navega una carpeta con subcarpetas, archivos de audio reconocidos y archivos no reconocidos
- **THEN** cada entrada muestra su ícono de tipo con la misma definición que el resto del chrome

#### Scenario: No se reserva memoria para imágenes que nadie dibuja
- **WHEN** la aplicación arranca
- **THEN** no reserva memoria gráfica para imágenes que ninguna pantalla llega a dibujar

### Requirement: Tamaño táctil mínimo de los controles
El sistema SHALL dar a todo control accionable por toque un área activa cuyo tamaño físico sobre la pantalla de la consola no sea menor que el mínimo táctil del lenguaje de diseño que la interfaz imita. El área activa MAY ser mayor que la forma dibujada del control.

#### Scenario: Controles de pista anterior y siguiente
- **WHEN** el usuario toca los controles de pista anterior o siguiente en Now Playing
- **THEN** el toque se registra dentro de un área que alcanza el mínimo táctil, sin exigir precisión mayor que la del resto de la interfaz

#### Scenario: Entradas de la barra de navegación
- **WHEN** el usuario toca una entrada de la barra de navegación para cambiar de pantalla
- **THEN** el toque se registra dentro de un área que alcanza el mínimo táctil

### Requirement: Ningún recurso gráfico se destruye con trabajo de dibujo pendiente
El sistema SHALL garantizar que ningún recurso gráfico se libere, se recree o se reutilice mientras siga pendiente el dibujo que lo referencia. Esto SHALL aplicar a toda ruta que destruya o recree un recurso, sin depender de qué otras condiciones se cumplan en esa ruta. La memoria que guarda la geometría de un fotograma SHALL contar como recurso gráfico: el sistema SHALL NOT escribir la geometría de un fotograma nuevo sobre memoria que el dibujo de un fotograma anterior todavía lee, tampoco después de un período en que la aplicación dejó de dibujar.

#### Scenario: Cambio repetido de track
- **WHEN** el usuario cambia de track muchas veces seguidas, alternando entre tracks con carátula embebida y tracks sin ella
- **THEN** la aplicación sigue reproduciendo y dibujando con normalidad, sin caerse en ninguna de las dos variantes

#### Scenario: Cambio de track sin artefactos visuales
- **WHEN** con el suavizado de bordes activo, el usuario abre Now Playing con una cola de al menos dos pistas y cambia de pista con R 70 veces seguidas, mirando cada vez los primeros instantes de la pista nueva
- **THEN** en ninguno de los 70 cambios aparecen triángulos o cuñas que crucen la pantalla, ni textos deformados en Up Next, en la barra de hints o en el nav rail

#### Scenario: Se reanuda el dibujo después de un stall
- **WHEN** la aplicación deja de dibujar durante un período largo (por ejemplo, mientras cierra una pista y abre la siguiente) y después vuelve a dibujar fotogramas
- **THEN** los primeros fotogramas después del stall se dibujan completos y con la misma geometría que los siguientes

#### Scenario: Un recurso se recrea a mitad de sesión
- **WHEN** el sistema necesita recrear un recurso gráfico durante la sesión, en vez de solo al arrancar o al salir
- **THEN** la recreación ocurre sin trabajo de dibujo pendiente sobre el recurso saliente, y la interfaz sigue dibujándose correctamente después

#### Scenario: Salida de la aplicación
- **WHEN** el usuario sale de la aplicación mientras hay un track en reproducción
- **THEN** la aplicación termina sin caerse, con y sin carátula cargada

### Requirement: La degradación por falta de recursos es definida y observable
El sistema SHALL seguir funcionando de forma definida cuando un recurso gráfico no puede crearse o cuando se agota la capacidad de dibujo de un fotograma, y SHALL NOT quedar en un estado donde partes de la interfaz desaparecen sin ninguna señal de por qué.

#### Scenario: El modo de suavizado pedido no está disponible
- **WHEN** el modo de suavizado de bordes solicitado no puede establecerse al arrancar
- **THEN** la aplicación arranca igual y queda utilizable, con las formas sin suavizar en lugar de no arrancar

#### Scenario: Un recurso de texto no puede cargarse
- **WHEN** alguna de las tipografías no puede cargarse al arrancar
- **THEN** la aplicación no se cae al intentar dibujar con ella

#### Scenario: Se agota la capacidad de dibujo del fotograma
- **WHEN** un fotograma pide dibujar más geometría de la que cabe en su capacidad
- **THEN** la condición queda registrada de forma observable durante las pruebas, en vez de manifestarse solo como elementos que faltan en pantalla

### Requirement: Los diálogos del sistema se componen fuera de la escena de dibujo del fotograma
El sistema SHALL componer todo diálogo provisto por el sistema operativo fuera de la escena de dibujo del fotograma de la aplicación, de modo que el diálogo se presente en pantalla y progrese según la interacción del usuario. La aplicación SHALL NOT pedir la composición de un diálogo mientras tiene una escena de dibujo abierta.

#### Scenario: Un diálogo del sistema se presenta al pedirlo
- **WHEN** la aplicación abre un diálogo provisto por el sistema mientras sigue dibujando fotogramas
- **THEN** el diálogo aparece en pantalla sobre el contenido de la aplicación

#### Scenario: Un diálogo del sistema responde y termina
- **WHEN** el usuario interactúa con un diálogo del sistema que está en pantalla
- **THEN** el diálogo progresa y termina, y la aplicación recibe su resultado y recupera el control

### Requirement: Ninguna operación que retenga el lazo de fotogramas deja la aplicación sin salida
El sistema SHALL garantizar que toda operación que retenga el lazo de fotogramas —esperar un diálogo del sistema, o cualquier trabajo que impida a la pantalla anterior seguir atendiendo al usuario— termine por sí sola aunque la condición que esperaba no llegue a cumplirse, devolviendo el control a la pantalla anterior y sin interrumpir la reproducción en curso. El sistema SHALL NOT quedar esperando indefinidamente una condición que no llega.

#### Scenario: La condición esperada no llega a cumplirse
- **WHEN** una operación que retiene el lazo de fotogramas espera una condición que no se cumple
- **THEN** la operación termina por sí sola y devuelve el control a la pantalla anterior, que vuelve a dibujarse

#### Scenario: La aplicación sigue dibujando mientras espera
- **WHEN** la aplicación está esperando el resultado de una operación que retiene el lazo de fotogramas
- **THEN** la pantalla sigue actualizándose en lugar de quedar congelada, de modo que el usuario puede ver que la aplicación sigue viva
