## Purpose

Define la vista de visualizador de Reproduciendo: cómo se abre y se recorre, qué dibuja cada uno de sus cinco estilos a partir de lo que está sonando, el vinilo que gira, y cómo se comporta en pausa, en silencio, con pistas mono y con la pantalla apagada.

## ADDED Requirements

### Requirement: Vista de visualizador con toggle
Reproduciendo SHALL ofrecer una vista de visualizador a pantalla completa, que se abre y se cierra con D-pad abajo. Mientras está abierta, SHALL mostrar en lugar de la carátula grande, el transporte en pantalla y la previsualización de próximas pistas: una cabecera con la carátula en pequeño, el título, el artista y el badge de formato; la zona del visualizador en el centro, con un indicador del estilo activo; y abajo la barra de progreso con el tiempo transcurrido y el total. En los estilos que dibujan el vinilo, la cabecera SHALL omitir la carátula en pequeño, porque la carátula ya es la etiqueta del disco. El nav rail y la leyenda de botones SHALL seguir visibles. Los botones físicos SHALL hacer en la vista de visualizador lo mismo que en la vista normal (pausa, pista anterior y siguiente, barajado, repetición, apagar pantalla y volver). Abrir, cerrar o recorrer la vista SHALL NOT cortar ni alterar la música.

#### Scenario: Abrir y cerrar el visualizador
- **WHEN** el usuario está en Reproduciendo, en la vista normal, con una pista sonando, pulsa D-pad abajo, y después vuelve a pulsarlo
- **THEN** la primera pulsación muestra la vista de visualizador con la cabecera, el estilo Espectro moviéndose con la música y la barra de progreso, sin cortar la música, y la segunda vuelve a la vista normal tal como estaba

#### Scenario: Los botones siguen funcionando en el visualizador
- **WHEN** el usuario, con la vista de visualizador abierta, pulsa el botón de confirmar, luego lo pulsa otra vez, luego R y luego Triángulo
- **THEN** la música se pausa y se reanuda, al pulsar R suena la pista siguiente con el visualizador todavía abierto y la cabecera mostrando esa pista, y Triángulo cambia el barajado como en la vista normal

#### Scenario: Volver desde el visualizador
- **WHEN** el usuario reproduce un archivo desde Carpetas, abre el visualizador y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a Carpetas, igual que desde la vista normal

#### Scenario: Buscar con la barra del visualizador
- **WHEN** el usuario, con el visualizador abierto, toca la barra de progreso en su punto medio
- **THEN** la canción salta aproximadamente a la mitad de su duración y el visualizador sigue moviéndose con lo que suena desde ese punto

#### Scenario: La cabecera en los estilos de vinilo
- **WHEN** el usuario, con el visualizador abierto en una pista con carátula, pasa del estilo Espectro al estilo Disco + espectro radial
- **THEN** en Espectro la cabecera muestra la carátula en pequeño, y en Disco + espectro radial la cabecera muestra el título, el artista y el badge sin la carátula en pequeño

### Requirement: Salto directo entre letras y visualizador
Desde la vista de letras, D-pad abajo SHALL abrir la vista de visualizador. Desde la vista de visualizador, D-pad arriba SHALL abrir la vista de letras. En ambos casos el cambio SHALL ser directo, sin pasar por la vista normal. Solo una de las dos vistas SHALL estar abierta a la vez.

#### Scenario: De las letras al visualizador y de vuelta
- **WHEN** el usuario, con la vista de letras abierta, pulsa D-pad abajo, y después pulsa D-pad arriba
- **THEN** la primera pulsación muestra el visualizador, y la segunda muestra la vista de letras, sin que en ningún momento aparezca la vista normal

#### Scenario: Cerrar el visualizador al que se llegó desde las letras
- **WHEN** el usuario llega al visualizador desde la vista de letras con D-pad abajo y vuelve a pulsar D-pad abajo
- **THEN** se muestra la vista normal

### Requirement: Recorrer los estilos
La vista de visualizador SHALL tener cinco estilos, en este orden: Espectro, Osciloscopio, VU, Disco + espectro radial y Disco que respira. D-pad derecha SHALL pasar al estilo siguiente y D-pad izquierda al anterior, dando la vuelta: después del último viene el primero, y antes del primero, el último. Deslizar el dedo hacia la izquierda sobre la zona del visualizador SHALL pasar al estilo siguiente, y hacia la derecha, al anterior. Un indicador SHALL mostrar cuántos estilos hay y cuál está activo. Cambiar de estilo SHALL NOT cambiar el punto de la canción ni la pista que suena. Fuera de la vista de visualizador, D-pad izquierda y derecha SHALL NOT cambiar el estilo.

#### Scenario: Recorrer los cinco estilos hacia adelante
- **WHEN** el usuario abre el visualizador y pulsa D-pad derecha cinco veces
- **THEN** pasa por Osciloscopio, VU, Disco + espectro radial y Disco que respira, y la quinta pulsación vuelve a Espectro, con el indicador marcando en cada paso el estilo visible

#### Scenario: Ir hacia atrás desde el primero
- **WHEN** el usuario, en el estilo Espectro, pulsa D-pad izquierda
- **THEN** se muestra el estilo Disco que respira

#### Scenario: Cambiar de estilo deslizando
- **WHEN** el usuario, en el estilo Espectro, desliza el dedo hacia la izquierda sobre la zona del visualizador
- **THEN** se muestra el estilo Osciloscopio, la música sigue en el mismo punto y la pista no cambia

#### Scenario: Las flechas laterales no hacen nada en la vista normal
- **WHEN** el usuario, en la vista normal de Reproduciendo, pulsa D-pad derecha y luego abre el visualizador
- **THEN** la vista normal no cambia al pulsar, y el visualizador se abre en el mismo estilo que tenía antes

### Requirement: La vista y el estilo se recuerdan en la sesión
La vista abierta (normal, letras o visualizador) y el estilo del visualizador SHALL mantenerse al cambiar de pista y al salir y volver a Reproduciendo durante la misma sesión. La app SHALL arrancar siempre en la vista normal y con el estilo Espectro. El estilo SHALL mantenerse también al cerrar el visualizador y volver a abrirlo.

#### Scenario: El visualizador se recuerda al navegar
- **WHEN** el usuario abre el visualizador, pasa al estilo VU, va a Ajustes con el nav rail y vuelve a Reproduciendo con el nav rail
- **THEN** Reproduciendo aparece con el visualizador abierto en el estilo VU

#### Scenario: El estilo se recuerda al cerrar la vista
- **WHEN** el usuario elige el estilo Osciloscopio, cierra el visualizador con D-pad abajo y lo vuelve a abrir
- **THEN** el visualizador se abre en el estilo Osciloscopio

#### Scenario: El estilo se mantiene al cambiar de pista
- **WHEN** el usuario, en el estilo Disco que respira, pasa a la pista siguiente con R
- **THEN** el visualizador sigue en Disco que respira, con la carátula de la pista nueva como etiqueta

#### Scenario: Al arrancar no se recuerda
- **WHEN** el usuario deja el visualizador abierto en el estilo VU, cierra la aplicación desde la LiveArea y la vuelve a abrir
- **THEN** al reproducir una pista, Reproduciendo aparece en la vista normal, y al abrir el visualizador aparece el estilo Espectro

### Requirement: Estilo Espectro
El estilo Espectro SHALL mostrar barras verticales que representan el nivel de bandas de frecuencia, de graves a la izquierda a agudos a la derecha, repartidas de forma que los graves y los medios ocupen una parte del ancho comparable a la de los agudos, como en un analizador de escala logarítmica. La altura de cada barra SHALL seguir el nivel de su banda: subir de inmediato y bajar de forma continua. Cada barra SHALL tener una marca de pico que se queda en el máximo reciente y luego cae. Las barras SHALL dibujarse con el color de acento de la pista.

#### Scenario: Un tono grave enciende la izquierda
- **WHEN** suena un tono puro de 100 Hz con el estilo Espectro abierto
- **THEN** las barras altas están en el lado izquierdo de la zona, y las del lado derecho quedan casi en cero

#### Scenario: Un tono agudo enciende la derecha
- **WHEN** suena un tono puro de 8 kHz con el estilo Espectro abierto
- **THEN** las barras altas están en la mitad derecha de la zona, y las del extremo izquierdo quedan casi en cero

#### Scenario: Las barras siguen a la música
- **WHEN** suena una canción con batería marcada de la biblioteca del usuario con el estilo Espectro abierto
- **THEN** las barras de los graves suben con cada golpe de bombo y bajan de forma continua entre golpes, y las marcas de pico quedan un instante arriba antes de caer

### Requirement: Estilo Osciloscopio
El estilo Osciloscopio SHALL mostrar como una línea continua la forma de onda de lo que suena, a lo ancho de la zona, centrada en vertical y con el color de acento de la pista. La amplitud de la línea SHALL ser proporcional a la de la señal, sin recortarse fuera de la zona. Con silencio, la línea SHALL quedar recta en el centro. Una onda periódica SHALL verse quieta y no desplazándose de un fotograma a otro.

#### Scenario: Un tono puro dibuja una onda quieta
- **WHEN** suena un tono puro de 440 Hz con el estilo Osciloscopio abierto
- **THEN** la zona muestra una onda senoidal regular que no se desliza hacia los lados de un fotograma a otro

#### Scenario: Silencio es una línea recta
- **WHEN** la música está en pausa con el estilo Osciloscopio abierto
- **THEN** después de un instante la zona muestra una línea recta horizontal en el centro

#### Scenario: Una señal fuerte no se sale
- **WHEN** suena un pasaje a todo volumen de una canción de la biblioteca del usuario con el estilo Osciloscopio abierto
- **THEN** la línea llega cerca de los bordes de la zona sin dibujarse fuera de ella

### Requirement: Estilo VU
El estilo VU SHALL mostrar dos medidores de nivel, uno para el canal izquierdo y otro para el derecho, rotulados L y R. Cada medidor SHALL indicar el nivel promedio de su canal y una marca de pico que se queda un instante en el máximo reciente y luego cae. La escala SHALL ser en decibeles, de modo que una música a volumen normal ocupe buena parte del medidor y no solo su extremo. Una pista mono SHALL mostrar el mismo nivel en los dos medidores.

#### Scenario: Canales distintos, medidores distintos
- **WHEN** suena un archivo estéreo con tono en el canal izquierdo y silencio en el derecho, con el estilo VU abierto
- **THEN** el medidor L marca nivel alto y el medidor R queda en cero

#### Scenario: Pista mono
- **WHEN** suena un archivo mono de `ux0:/pruebas-eleven/formatos-mono/` con el estilo VU abierto
- **THEN** los dos medidores marcan el mismo nivel

#### Scenario: La música normal no queda pegada abajo
- **WHEN** suena una canción de la biblioteca del usuario con el estilo VU abierto
- **THEN** los medidores se mueven en la mitad superior de su recorrido durante los pasajes fuertes y bajan en los suaves

### Requirement: El vinilo
Los estilos Disco + espectro radial y Disco que respira SHALL dibujar un vinilo: un disco oscuro con surcos concéntricos y, en el centro, la carátula de la pista recortada en círculo como etiqueta, con un agujero central. El borde del disco y el de la etiqueta SHALL verse suavizados, sin escalones. Si la pista no tiene carátula, la etiqueta SHALL ser un círculo liso, del color de acento de la pista. El vinilo SHALL girar a velocidad constante mientras la música suena. Al pausar, SHALL frenar de forma continua hasta detenerse en un par de segundos; al reanudar, SHALL acelerar de forma continua hasta su velocidad. Al cambiar de pista, la etiqueta SHALL pasar a ser la carátula de la pista nueva sin que el disco deje de girar.

#### Scenario: El vinilo gira con la carátula como etiqueta
- **WHEN** el usuario reproduce un FLAC de `ux0:/pruebas-eleven/flac/` con carátula y abre el estilo Disco + espectro radial
- **THEN** se ve un disco oscuro con surcos, la carátula de la pista en círculo en el centro y un agujero central, y el disco gira

#### Scenario: Pausar frena el disco
- **WHEN** el usuario, con el vinilo girando, pulsa el botón de confirmar para pausar, y después vuelve a pulsarlo
- **THEN** al pausar el disco frena poco a poco hasta detenerse, sin parar en seco, y al reanudar vuelve a tomar velocidad poco a poco

#### Scenario: Pista sin carátula
- **WHEN** el usuario reproduce un archivo de `ux0:/pruebas-eleven/tracker/` y abre el estilo Disco que respira
- **THEN** se ve el vinilo con una etiqueta lisa del color de acento en lugar de una carátula, y el disco gira

#### Scenario: El borde se ve suave
- **WHEN** el usuario mira de cerca el borde del vinilo y el de la etiqueta mientras giran
- **THEN** los bordes se ven como curvas lisas, sin escalones ni parpadeo

### Requirement: Estilo Disco + espectro radial
El estilo Disco + espectro radial SHALL dibujar el vinilo en el centro de la zona y, alrededor de su borde, barras radiales que apuntan hacia afuera y representan las mismas bandas de frecuencia que el estilo Espectro, con el mismo comportamiento de subida y caída y con el color de acento de la pista. Las barras SHALL NOT girar con el disco.

#### Scenario: Las barras rodean al disco
- **WHEN** suena una canción de la biblioteca del usuario con el estilo Disco + espectro radial abierto
- **THEN** alrededor del vinilo que gira se ven barras que salen hacia afuera y se mueven con la música, y su posición alrededor del disco no rota con él

### Requirement: Estilo Disco que respira
El estilo Disco que respira SHALL dibujar el vinilo, que aumenta un poco de tamaño con la energía de los graves y vuelve de forma continua a su tamaño en reposo, y detrás de él un halo del color de acento de la pista cuya intensidad y alcance crecen con el nivel de lo que suena. En silencio o en pausa, el vinilo SHALL quedar en su tamaño en reposo y el halo apagado o tenue.

#### Scenario: El disco late con el bombo
- **WHEN** suena una canción con bombo marcado de la biblioteca del usuario con el estilo Disco que respira abierto
- **THEN** el vinilo se agranda levemente con cada golpe de bombo y vuelve a su tamaño entre golpes, y el halo se intensifica con los pasajes fuertes

#### Scenario: En pausa el disco descansa
- **WHEN** el usuario pausa la música con el estilo Disco que respira abierto
- **THEN** después de un instante el vinilo queda en su tamaño en reposo, detenido, con el halo tenue

### Requirement: El visualizador está sincronizado con lo que se oye
Lo que muestra el visualizador SHALL corresponder a lo que está saliendo por los altavoces o los auriculares en ese momento, y no a lo que la aplicación acaba de decodificar. El desfase entre un sonido y su efecto en el visualizador SHALL NOT ser perceptible a simple vista, ni adelantado ni atrasado.

#### Scenario: Pulsos que coinciden
- **WHEN** suena un archivo de prueba con un pitido corto cada segundo y silencio entre ellos, con el estilo Espectro abierto
- **THEN** las barras suben en el mismo momento en que se oye cada pitido, y no antes ni después de forma perceptible

#### Scenario: Después de buscar
- **WHEN** el usuario busca otro punto de la canción con la barra de progreso, con el visualizador abierto
- **THEN** el visualizador muestra lo que suena desde el punto nuevo, sin mostrar antes un resto de lo que sonaba en el punto anterior

### Requirement: Pausa, silencio y cambio de pista
Al pausar, al terminar la cola o mientras la pista tiene silencio, los niveles que muestra el visualizador SHALL caer a cero de forma continua y quedarse ahí, sin dejar barras, medidores ni formas de onda congeladas en el último valor. Al cambiar de pista, el visualizador SHALL seguir moviéndose con la pista nueva.

#### Scenario: Pausar baja las barras
- **WHEN** el usuario pausa la música con el estilo Espectro abierto y espera dos segundos
- **THEN** las barras bajan de forma continua hasta cero y se quedan ahí

#### Scenario: Al terminar la cola
- **WHEN** termina la última pista de una cola sin repetición con el estilo VU abierto
- **THEN** los medidores bajan a cero y se quedan ahí

### Requirement: El visualizador funciona con todos los formatos
El visualizador SHALL funcionar con todos los formatos que la aplicación reproduce (FLAC, MP3, OGG, Opus, WAV y módulos tracker), con pistas mono y estéreo y con las frecuencias de muestreo que esos formatos usan, mostrando en cada caso el mismo comportamiento de los estilos. Una pista mono SHALL NOT dañar la memoria ni detener la aplicación.

#### Scenario: Un archivo de cada formato
- **WHEN** el usuario reproduce cada archivo de `ux0:/pruebas-eleven/formatos/` con el visualizador abierto
- **THEN** en todos el visualizador se mueve con la música, sin cortes en el audio ni cierres de la aplicación

#### Scenario: Pistas mono de principio a fin
- **WHEN** el usuario reproduce de principio a fin cada archivo de `ux0:/pruebas-eleven/formatos-mono/` con el visualizador abierto
- **THEN** cada pista se escucha completa, el visualizador se mueve con ella y la aplicación sigue respondiendo al terminar

### Requirement: El visualizador no degrada la reproducción ni la interfaz
Con el visualizador abierto, en cualquiera de sus estilos, la música SHALL sonar sin cortes ni ruidos y la interfaz SHALL responder con la misma fluidez que en la vista normal. Con la pantalla apagada, la aplicación SHALL NOT hacer trabajo de visualizador, y al encender la pantalla con el visualizador abierto SHALL mostrar lo que suena en ese momento.

#### Scenario: Una canción entera en el estilo más pesado
- **WHEN** el usuario escucha una canción completa de la biblioteca del usuario con el estilo Disco + espectro radial abierto
- **THEN** la canción suena sin cortes, el disco gira sin tirones y la barra de progreso avanza con fluidez

#### Scenario: Encender la pantalla con el visualizador abierto
- **WHEN** el usuario, con el visualizador abierto y música sonando, apaga la pantalla con START, espera un minuto y la enciende con el botón PS
- **THEN** la aplicación sigue en el visualizador, la música sigue sonando, y el visualizador se mueve con lo que suena en ese momento

### Requirement: Usar el visualizador no acumula memoria
Abrir y cerrar el visualizador, recorrer sus estilos y cambiar de pista con él abierto SHALL NOT hacer crecer la memoria en uso de la aplicación.

#### Scenario: Recorrer los estilos muchas veces
- **WHEN** con el log de desarrollo prendido, el usuario abre el visualizador, pulsa D-pad derecha cincuenta veces, lo cierra, va a Carpetas con el nav rail, y repite esa secuencia diez veces
- **THEN** el `heap_kb` de la última línea `screen` con `to=folders` es igual al de la primera, con una diferencia de no más de unos pocos KB
