## MODIFIED Requirements

### Requirement: Color de acento fijo cuando no hay carátula disponible
El sistema SHALL usar el color de acento fijo por defecto cuando el track en reproducción no tiene carátula embebida, y SHALL usar ese mismo color fijo cuando no hay ningún track cargado. El sistema SHALL NOT usar el color fijo por defecto para un track que sí tiene carátula embebida y legible, aunque esa carátula no tenga color: ese caso lo cubre el requirement del acento neutro.

#### Scenario: Track sin carátula embebida
- **WHEN** el track en reproducción no tiene carátula embebida (por ejemplo, un WAV o un archivo de tracker MOD/IT/S3M)
- **THEN** el color de acento de la interfaz es el color fijo por defecto

#### Scenario: Ningún track cargado
- **WHEN** no hay ningún track cargado en la sesión de audio
- **THEN** el color de acento de la interfaz es el color fijo por defecto

#### Scenario: Carátula en blanco y negro no usa el color fijo
- **WHEN** el usuario reproduce en la consola un track de *EASY* (LE SSERAFIM), cuya carátula es blanca con relieve y no tiene color
- **THEN** el botón de play de Now Playing, la barra de progreso y el indicador del nav rail no muestran el naranja fijo por defecto

### Requirement: El color derivado se mantiene legible sobre el fondo oscuro fijo
El sistema SHALL ajustar el tono dominante extraído de la carátula a un rango de saturación y luminosidad que mantenga el texto y los íconos dibujados con ese acento legibles sobre el fondo oscuro fijo de la interfaz, independientemente de qué tan oscura, clara o desaturada sea la carátula de origen.

#### Scenario: Carátula predominantemente oscura
- **WHEN** la carátula del track en reproducción es predominantemente oscura o de bajo contraste
- **THEN** el color de acento resultante mantiene suficiente luminosidad y saturación para distinguirse del fondo y del texto secundario

#### Scenario: Carátula predominantemente desaturada (escala de grises)
- **WHEN** la carátula del track en reproducción es mayormente monocromática o de muy baja saturación
- **THEN** el color de acento resultante no queda indistinguible del gris de fondo ni del texto secundario existente: o es el color de un detalle de la carátula, ajustado como cualquier acento derivado, o es el acento neutro claro

## ADDED Requirements

### Requirement: Un detalle de color pequeño en la carátula define el acento
El sistema SHALL derivar el acento del color de la carátula aunque ese color ocupe una parte pequeña de la imagen (del orden del 1 % de la superficie), siempre que ese color sea claro y no un tinte apenas perceptible. Una carátula que hoy ya define el acento SHALL seguir dando exactamente el mismo acento.

#### Scenario: Título de color sobre una carátula gris
- **WHEN** el usuario reproduce en la consola un track de *The Book of Us : Entropy* (DAY6), cuya carátula es gris con el título en rojo
- **THEN** el acento de la interfaz es un rojo, visible en el botón de play de Now Playing, la barra de progreso y el indicador del nav rail

#### Scenario: Cromado con tinte frío
- **WHEN** el usuario reproduce en la consola un track de *2* (i-dle), cuya carátula es un logo cromado de tinte azulado
- **THEN** el acento de la interfaz es un azul claro, no el naranja fijo ni el acento neutro

#### Scenario: Carátulas que ya tenían color no cambian
- **WHEN** el usuario reproduce en la consola un track de una carátula que antes de este cambio ya teñía la interfaz (por ejemplo, *DAYDREAM* o *Shoot Me : Youth, Part. 1* de DAY6)
- **THEN** el acento es el mismo color que mostraba la versión anterior de la app con ese mismo track

### Requirement: Acento neutro para carátulas sin color
Cuando el track en reproducción tiene una carátula embebida pero no hay en ella ningún color usable (blanco, negro, grises o tintes apenas perceptibles), el sistema SHALL usar un acento neutro claro, sin tono, en lugar del color fijo por defecto. El acento neutro SHALL distinguirse del fondo oscuro fijo y del texto secundario, y SHALL aplicarse igual que cualquier acento derivado: en las tres pantallas de nivel superior y en el nav rail.

#### Scenario: Carátula blanca
- **WHEN** el usuario reproduce en la consola un track de *EASY* (LE SSERAFIM)
- **THEN** el botón de play de Now Playing, la barra de progreso y el indicador del nav rail se muestran en un tono claro casi blanco, sin color

#### Scenario: Carátula de fotografía en blanco y negro
- **WHEN** el usuario reproduce en la consola un track de *We are i-dle* (i-dle), cuya carátula es negra con letras blancas
- **THEN** el acento de la interfaz es el mismo acento neutro claro que con *EASY*

#### Scenario: Cambio entre una carátula sin color y un track sin carátula
- **WHEN** el usuario reproduce un track de *EASY* y después abre un archivo sin carátula embebida (por ejemplo, un MOD)
- **THEN** el acento pasa del neutro claro al naranja fijo por defecto

#### Scenario: El acento neutro se mantiene al cambiar de pantalla
- **WHEN** un track de *EASY* está en reproducción y el usuario pasa a Folders y luego a Settings con el nav rail
- **THEN** el elemento seleccionado en Folders, el interruptor activo en Settings y el indicador del nav rail usan el mismo acento neutro claro, y cada uno se distingue a simple vista de los elementos inactivos de su pantalla

### Requirement: El contenido dibujado sobre el acento se mantiene legible
Cuando la interfaz dibuja un ícono sobre una superficie rellena con el color de acento (el botón de play/pausa de Now Playing y el del mini player), el sistema SHALL elegir el color de ese ícono de modo que se distinga claramente del relleno, con cualquier acento: el fijo, uno derivado de la carátula o el neutro.

#### Scenario: Símbolo de play sobre el acento neutro
- **WHEN** un track de *EASY* está en reproducción y el usuario lo pausa en Now Playing
- **THEN** el símbolo de play dentro del botón relleno con el acento neutro se ve con claridad, en un color oscuro

#### Scenario: Símbolo de pausa en el mini player sobre el acento neutro
- **WHEN** un track de *EASY* está en reproducción y el usuario va a Folders, donde aparece el mini player
- **THEN** el símbolo de pausa dentro del botón relleno del mini player se ve con claridad

#### Scenario: Símbolo de play sobre un acento de color
- **WHEN** el track en reproducción tiene una carátula con color (por ejemplo, *Entropy*) o no tiene carátula
- **THEN** el símbolo de play/pausa dentro del botón relleno se ve igual que en la versión anterior de la app
