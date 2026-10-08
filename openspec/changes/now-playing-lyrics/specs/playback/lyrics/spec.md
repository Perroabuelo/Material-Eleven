## Purpose

Define de dónde sale la letra de la pista que suena, cómo se interpreta su texto y sus tiempos, y qué garantías hay sobre su codificación y su memoria, para que Reproduciendo pueda mostrarla sincronizada con la canción.

## ADDED Requirements

### Requirement: La letra sale de un .lrc hermano o del tag embebido
Al abrir una pista, el sistema SHALL buscar su letra en dos fuentes: un archivo con el mismo nombre que la pista y extensión `.lrc`, en la misma carpeta, y la letra embebida en la pista (el campo `LYRICS` o `UNSYNCEDLYRICS` de los comentarios Vorbis en FLAC, OGG y Opus, y el marco `USLT` de ID3v2 en MP3). Si las dos fuentes tienen letra, SHALL usar la que tenga tiempos; si ambas tienen tiempos o ninguna los tiene, SHALL usar el `.lrc`. Una fuente vacía, o que solo contiene etiquetas de metadatos, SHALL contar como sin letra. Buscar la letra SHALL NOT depender de los ajustes que deciden si se leen las carátulas.

#### Scenario: Letra en un .lrc hermano
- **WHEN** el usuario reproduce `01 - i-dle - TOMBOY.flac`, que tiene al lado `01 - i-dle - TOMBOY.lrc`, y abre la vista de letras
- **THEN** la vista muestra la letra de ese `.lrc`, empezando por "Ah-ah-ah-ah-ah"

#### Scenario: Letra embebida en un MP3
- **WHEN** el usuario reproduce un MP3 de *KPop Demon Hunters* que trae la letra en un `USLT` y no tiene `.lrc` al lado, y abre la vista de letras
- **THEN** la vista muestra esa letra

#### Scenario: Gana la letra con tiempos
- **WHEN** una pista tiene la letra embebida sin tiempos y además un `.lrc` hermano con tiempos (prueba en PC con las dos fuentes)
- **THEN** la letra elegida es la del `.lrc`

#### Scenario: A igualdad gana el .lrc
- **WHEN** una pista tiene la letra embebida y un `.lrc` hermano, ambos sin tiempos (prueba en PC)
- **THEN** la letra elegida es la del `.lrc`

#### Scenario: Una fuente vacía no tapa a la otra
- **WHEN** una pista tiene un `.lrc` hermano vacío o con solo `[ti:]` y `[ar:]`, y la letra embebida tiene texto (prueba en PC)
- **THEN** la letra elegida es la embebida

#### Scenario: Pista sin ninguna letra
- **WHEN** el usuario reproduce `TWICE - TAKEDOWN (JEONGYEON, JIHYO, CHAEYOUNG).mp3`, cuyo `USLT` está vacío y no tiene `.lrc` al lado
- **THEN** el sistema considera que la pista no tiene letra

#### Scenario: La letra no depende del ajuste de carátulas
- **WHEN** el usuario desactiva en Ajustes la lectura de metadatos de MP3 y reproduce un MP3 de *KPop Demon Hunters* con `USLT`
- **THEN** la vista de letras sigue mostrando la letra

### Requirement: La letra con tiempos se interpreta por línea
El sistema SHALL tratar una letra como sincronizada cuando tiene al menos una línea con una marca de tiempo válida `[mm:ss]`, `[mm:ss.x]`, `[mm:ss.xx]` o `[mm:ss.xxx]`, con minutos de una o más cifras y segundos menores que 60; los decimales más allá de los milésimos SHALL ignorarse. Una línea con varias marcas al inicio SHALL aparecer una vez por cada marca. Las líneas SHALL ordenarse por tiempo, aunque en el archivo no lo estén. Las marcas de tiempo por palabra (`<mm:ss.xx>`) dentro de una línea SHALL quitarse del texto, y la línea SHALL usar su marca de inicio. Una línea cuya marca no es válida, o que no tiene marca, SHALL descartarse en una letra sincronizada, sin impedir que se muestren las demás.

#### Scenario: Letra bien formada
- **WHEN** se interpreta `01 bien.lrc` de las pruebas (prueba en PC)
- **THEN** el resultado tiene cinco líneas, en este orden: "Primera línea" a 0 ms, "Segunda línea" a 4000 ms, "Tercera, con acentos: ñandú" a 8500 ms, y "Una línea con dos marcas" a 12000 ms y a 16000 ms

#### Scenario: Marcas inválidas
- **WHEN** se interpreta `02 marcas invalidas.lrc` (prueba en PC)
- **THEN** quedan "Esta marca es válida" a 0 ms, "Sin centésimas" a 5000 ms y "Demasiados decimales" a 8123 ms, y se descartan las líneas con `[99:99.99]`, `[0a:1b.2c]` y `[-00:01.00]`

#### Scenario: Corchete sin cerrar
- **WHEN** se interpreta `03 corchete sin cerrar.lrc` (prueba en PC)
- **THEN** quedan "Bien" a 0 ms y "Después del error" a 8000 ms, sin que la línea rota ni la etiqueta `[ar:` sin cerrar aparezcan como texto

#### Scenario: Líneas desordenadas
- **WHEN** se interpreta una letra cuyas marcas aparecen como 00:10, 00:02, 00:05 (prueba en PC)
- **THEN** las líneas quedan en el orden 00:02, 00:05, 00:10

#### Scenario: Marcas por palabra
- **WHEN** se interpreta la línea `[00:12.30]<00:12.30>Hola <00:12.80>mundo` (prueba en PC)
- **THEN** queda una línea "Hola mundo" a 12300 ms

### Requirement: Las etiquetas LRC conocidas se descartan y lo demás es texto
El sistema SHALL tratar como etiqueta de metadatos, y no mostrar, solo una línea que consiste en una etiqueta con una de las claves `ti`, `ar`, `al`, `au`, `by`, `length`, `offset`, `re` o `ve`, sin importar mayúsculas. Cualquier otra línea entre corchetes, como un encabezado de sección `[Verse: Rumi, Zoey]`, SHALL conservarse como texto.

#### Scenario: Encabezados de una letra de Genius
- **WHEN** se interpreta una letra sin tiempos que empieza con `[헌트릭스 "Golden" 가사]`, una línea vacía y `[Verse: Rumi, Zoey, Mira, All]` (prueba en PC)
- **THEN** las dos líneas entre corchetes se conservan como texto, marcadas como encabezado

#### Scenario: Etiquetas de cabecera de LRCLIB
- **WHEN** se interpreta un `.lrc` que empieza con `[ti:TOMBOY]`, `[ar:i-dle]`, `[al:I NEVER DIE]` y `[by:LRCLIB]` (prueba en PC)
- **THEN** ninguna de esas cuatro líneas aparece en el texto de la letra

### Requirement: El desfase del archivo se respeta
Si una letra sincronizada trae una etiqueta `[offset:N]`, con N en milisegundos y signo opcional, el sistema SHALL mostrar cada línea N milisegundos antes de su marca cuando N es positivo, y |N| milisegundos después cuando es negativo, sin llevar ninguna línea a un tiempo menor que cero. Un valor de desfase que no es un número SHALL ignorarse.

#### Scenario: Desfase positivo
- **WHEN** se interpreta una letra con `[offset:+500]` y una línea en `[00:10.00]` (prueba en PC)
- **THEN** esa línea queda a 9500 ms

#### Scenario: Desfase negativo
- **WHEN** se interpreta una letra con `[offset:-250]` y una línea en `[00:10.00]` (prueba en PC)
- **THEN** esa línea queda a 10250 ms

#### Scenario: Desfase que lleva bajo cero
- **WHEN** se interpreta una letra con `[offset:1000]` y una línea en `[00:00.40]` (prueba en PC)
- **THEN** esa línea queda a 0 ms

### Requirement: Una letra sin tiempos se conserva tal cual
Cuando una letra no tiene ninguna marca de tiempo válida, el sistema SHALL conservar todas sus líneas en su orden, incluidas las vacías que separan estrofas, quitando solo las etiquetas de metadatos conocidas y los espacios al final de cada línea.

#### Scenario: Estrofas separadas
- **WHEN** se interpreta una letra sin tiempos con dos estrofas separadas por una línea vacía (prueba en PC)
- **THEN** el resultado conserva las líneas de las dos estrofas y la línea vacía entre ellas, y se marca como no sincronizada

### Requirement: La codificación de un .lrc se detecta por capas
Al leer un `.lrc`, el sistema SHALL convertir su contenido a UTF-8 así: si empieza con el BOM de UTF-8, SHALL quitarlo; si empieza con el BOM de UTF-16 (little o big endian), SHALL convertirlo desde UTF-16; si no tiene BOM y es UTF-8 válido, SHALL usarlo tal cual; en cualquier otro caso, SHALL interpretarlo como Windows-1252. El resultado SHALL ser siempre UTF-8 válido, sea cual sea el contenido del archivo. Los saltos de línea `\n`, `\r\n` y `\r` SHALL tratarse igual. La letra embebida SHALL tomarse como UTF-8, que es como la entregan sus formatos.

#### Scenario: UTF-8 con BOM
- **WHEN** se interpreta `06 bom utf8.lrc` (prueba en PC)
- **THEN** la primera línea no empieza con caracteres invisibles y las líneas son las mismas que en `01 bien.lrc`

#### Scenario: UTF-16 con BOM
- **WHEN** se interpreta el contenido de `01 bien.lrc` guardado como UTF-16 little endian con BOM, y también como big endian (prueba en PC)
- **THEN** en ambos casos las líneas son las mismas que en `01 bien.lrc`

#### Scenario: Archivo en Latin-1
- **WHEN** el usuario reproduce `07 no utf8.wav` de las pruebas y abre la vista de letras
- **THEN** la primera línea se lee "Línea en Latin-1: canción, ñandú", con sus acentos y su eñe

#### Scenario: Fin de línea de Windows
- **WHEN** se interpreta `08 crlf.lrc` (prueba en PC)
- **THEN** las líneas son las mismas que en `01 bien.lrc`, sin un `\r` al final de ninguna

#### Scenario: Bytes arbitrarios
- **WHEN** se interpreta un bloque de bytes aleatorios, con y sin BOM (prueba en PC con sanitizers)
- **THEN** la interpretación termina sin errores de memoria y todo el texto resultante es UTF-8 válido

### Requirement: Una letra demasiado grande no se carga
El sistema SHALL ignorar, como si no existiera, un `.lrc` o una letra embebida de más de 64 KB, y SHALL NOT leer más de 64 KB para decidirlo.

#### Scenario: Archivo de más de 64 KB
- **WHEN** una pista tiene al lado un `.lrc` de 100 KB y no tiene letra embebida (prueba en consola: se copia el archivo junto a `01 bien.wav` en lugar de su `.lrc`)
- **THEN** la vista de letras muestra que la pista no tiene letra, y la reproducción no se retrasa

#### Scenario: Línea muy larga
- **WHEN** el usuario reproduce `04 linea larga.wav`, cuyo `.lrc` tiene una línea de casi 3 KB, y abre la vista de letras
- **THEN** la línea se muestra completa repartida en varios renglones, y la aplicación sigue respondiendo

### Requirement: La letra se libera con la pista
El sistema SHALL tomar la memoria de la letra al abrir la pista y SHALL devolverla al cambiar de pista, por cualquier camino, y cuando la pista no llega a abrirse, igual que el resto de los recursos de la pista.

#### Scenario: Cambiar de pista con letras no acumula memoria
- **WHEN** el usuario abre el overlay de debug, reproduce la carpeta `I NEVER DIE - i-dle` (pistas con y sin `.lrc`), abre la vista de letras, anota el heap en uso, pasa de pista con R treinta veces y vuelve a la primera pista
- **THEN** el heap en uso que muestra el overlay es el mismo que el anotado, con una diferencia de no más de unos pocos KB

#### Scenario: El parser no pierde memoria
- **WHEN** se interpretan y liberan todos los `.lrc` de prueba y el bloque de bytes aleatorios (prueba en PC con AddressSanitizer)
- **THEN** la prueba no reporta fugas
