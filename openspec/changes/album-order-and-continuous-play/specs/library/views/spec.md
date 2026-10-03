## MODIFIED Requirements

### Requirement: Reproducir desde una vista convierte esa vista en la cola
El sistema SHALL tomar como cola de reproducción la lista de pistas de la vista desde la que el usuario inició la reproducción, en el orden en que esa vista las muestra. Las acciones de pista siguiente y anterior SHALL recorrer esa cola, y no la carpeta en la que se encuentre el archivo reproducido.

La única excepción es reproducir desde dentro de un álbum o de un artista con el ajuste "Al terminar un álbum o artista" en "Seguir con el siguiente". En ese caso la cola SHALL ser toda la biblioteca:
- Dentro de un álbum: agrupada por álbum, con los álbumes en el orden de la vista Álbumes y las pistas de cada uno en el orden del disco.
- Dentro de un artista: agrupada por artista, con los artistas en el orden de la vista Artistas y las pistas de cada uno en el orden que muestra ese artista.

La reproducción SHALL empezar por la pista elegida. Con el ajuste en "Repetirlo", la cola SHALL ser solo la vista abierta, como hasta ahora. En las vistas Canciones y Añadidos recientemente el ajuste SHALL NOT cambiar nada.

#### Scenario: Reproducir desde un álbum
- **WHEN** el usuario abre un álbum en la biblioteca y reproduce una de sus pistas
- **THEN** la pista siguiente es la siguiente del álbum, aunque los archivos del álbum estén repartidos en carpetas distintas

#### Scenario: Reproducir desde el navegador de carpetas
- **WHEN** el usuario reproduce un archivo desde el navegador de carpetas
- **THEN** la cola sigue siendo la carpeta que estaba viendo, igual que antes de existir la biblioteca

#### Scenario: La cola sobrevive al cambio de pantalla
- **WHEN** el usuario reproduce desde una vista de biblioteca y luego se va a otra pantalla usando el nav rail
- **THEN** la reproducción continúa y la pista siguiente sigue saliendo de la cola con la que empezó

#### Scenario: Repetir el álbum
- **WHEN** el ajuste está en "Repetirlo", el usuario reproduce la última pista de un álbum y la deja terminar
- **THEN** suena la primera pista de ese mismo álbum

#### Scenario: Seguir con el siguiente álbum
- **WHEN** el ajuste está en "Seguir con el siguiente", el usuario reproduce la última pista de un álbum y la deja terminar
- **THEN** suena la primera pista del álbum que sigue en la vista Álbumes, y "A continuación" lo anunciaba antes de que terminara

#### Scenario: Seguir con el siguiente artista
- **WHEN** el ajuste está en "Seguir con el siguiente", el usuario reproduce la última pista de un artista y la deja terminar
- **THEN** suena la primera pista del artista que sigue en la vista Artistas

#### Scenario: Del último grupo vuelve al primero
- **WHEN** el ajuste está en "Seguir con el siguiente" y termina la última pista del último álbum de la vista Álbumes
- **THEN** suena la primera pista del primer álbum de la vista Álbumes

#### Scenario: Seguir con el siguiente y barajar
- **WHEN** el ajuste está en "Seguir con el siguiente", el usuario reproduce desde un álbum y enciende el barajado
- **THEN** las pistas siguientes salen de toda la biblioteca, y no solo de ese álbum

#### Scenario: El ajuste no cambia la vista Canciones
- **WHEN** el ajuste está en "Seguir con el siguiente" y el usuario reproduce desde la vista Canciones
- **THEN** la cola es la vista Canciones en su orden, igual que con "Repetirlo"

### Requirement: Las listas de la biblioteca muestran la carátula del álbum
El sistema SHALL mostrar en las vistas de biblioteca la carátula del álbum de cada entrada cuando exista, y un marcador por defecto coherente con el resto de la interfaz cuando no exista. En la vista Artistas, cada artista SHALL mostrar la carátula de la primera de sus pistas, en el orden que muestra ese artista, que tenga carátula disponible. La entrada "Desconocido" de la vista Artistas SHALL mostrar el marcador por defecto. La presentación de una lista SHALL NOT quedar retenida esperando a que sus carátulas estén disponibles.

#### Scenario: Álbum con carátula
- **WHEN** el usuario mira la vista de álbumes y alguno de ellos tiene carátula
- **THEN** esa entrada muestra su carátula

#### Scenario: Álbum sin carátula
- **WHEN** el usuario mira una entrada cuya carátula no está disponible
- **THEN** esa entrada muestra el marcador por defecto, y la lista se ve completa igual

#### Scenario: Recorrer una lista larga
- **WHEN** el usuario recorre rápidamente una lista larga de la biblioteca
- **THEN** la lista responde al desplazamiento sin quedarse detenida esperando carátulas

#### Scenario: Artista con carátula
- **WHEN** el usuario mira la vista Artistas y un artista tiene al menos un álbum con carátula
- **THEN** la fila de ese artista muestra una de esas carátulas, la misma cada vez que vuelve a la vista

#### Scenario: Artista sin ninguna carátula
- **WHEN** ninguna pista de un artista tiene carátula disponible
- **THEN** la fila de ese artista muestra el marcador por defecto

#### Scenario: El cubo de artistas desconocidos
- **WHEN** el usuario mira la entrada "Desconocido" de la vista Artistas
- **THEN** esa fila muestra el marcador por defecto

## ADDED Requirements

### Requirement: Las pistas de un álbum o de un artista siguen el orden del disco
Dentro de un álbum, el sistema SHALL ordenar las pistas por número de disco, luego por número de pista y luego por título. Una pista sin número de disco SHALL ordenarse como disco 1. Las pistas sin número de pista SHALL ir después de las numeradas de su disco, ordenadas por título. Dentro de un artista, el sistema SHALL agrupar las pistas por álbum, con los álbumes en orden alfabético sin distinguir mayúsculas y las pistas sin álbum al final, y SHALL ordenar cada álbum como se ordena dentro de él. Entre dos pistas que empatan en todo lo anterior, la ruta SHALL decidir, para que el orden sea estable. Las vistas Canciones y Añadidos recientemente SHALL conservar su orden actual.

#### Scenario: Álbum con números de pista
- **WHEN** el usuario abre un álbum cuyas pistas traen los números 1 a 10 y sus títulos no están en orden alfabético
- **THEN** las pistas se muestran y suenan del 1 al 10

#### Scenario: Álbum de dos discos
- **WHEN** el usuario abre un álbum con las pistas 1 a 8 del disco 1 y 1 a 6 del disco 2
- **THEN** se muestran primero las 8 del disco 1 y después las 6 del disco 2, cada grupo en su orden

#### Scenario: Pistas sin número
- **WHEN** un álbum tiene pistas numeradas y otras sin número
- **THEN** las numeradas van primero en su orden y las sin número van al final, por título; lo verifica un test en PC sobre el comparador

#### Scenario: Dentro de un artista
- **WHEN** el usuario abre un artista con dos álbumes, "Beta" y "Alfa", y algunas pistas sin álbum
- **THEN** se muestran las pistas de "Alfa" en su orden, después las de "Beta" en su orden, y al final las que no traen álbum

#### Scenario: La vista Canciones no cambia
- **WHEN** el usuario mira la vista Canciones
- **THEN** las pistas siguen ordenadas por título, aunque traigan número de pista
