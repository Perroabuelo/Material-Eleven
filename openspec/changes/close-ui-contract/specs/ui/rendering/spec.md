## Purpose

Fija cómo se dibujan las formas, los íconos y los controles de la interfaz, qué tamaño deben tener para poder tocarse, y cómo se manejan los recursos gráficos durante la sesión, para que la interfaz se vea nítida y no vuelva a caerse por la clase de fallo de GPU que ya se observó dos veces en hardware.

## ADDED Requirements

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
El sistema SHALL garantizar que ningún recurso gráfico se libere, se recree o se reutilice mientras siga pendiente el dibujo que lo referencia. Esto SHALL aplicar a toda ruta que destruya o recree un recurso, sin depender de qué otras condiciones se cumplan en esa ruta.

#### Scenario: Cambio repetido de track
- **WHEN** el usuario cambia de track muchas veces seguidas, alternando entre tracks con carátula embebida y tracks sin ella
- **THEN** la aplicación sigue reproduciendo y dibujando con normalidad, sin caerse en ninguna de las dos variantes

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
