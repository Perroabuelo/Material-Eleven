## MODIFIED Requirements

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
