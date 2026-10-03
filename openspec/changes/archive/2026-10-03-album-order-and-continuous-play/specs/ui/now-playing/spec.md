## ADDED Requirements

### Requirement: La previsualización muestra la carátula de las próximas pistas
Cada pista de la previsualización de próximas pistas SHALL mostrar la carátula de su álbum cuando la cola vino de la biblioteca y la biblioteca tiene esa carátula guardada. Cuando la cola vino del navegador de carpetas, o la carátula no está disponible, SHALL mostrar el marcador por defecto que muestra hoy. Obtener esas carátulas SHALL NOT retener la pantalla ni el cambio de pista: una carátula que todavía no llegó se muestra con el marcador y aparece en cuanto está disponible.

#### Scenario: Cola de biblioteca con carátulas
- **WHEN** el usuario reproduce desde un álbum de la biblioteca que tiene carátula y quedan pistas por delante
- **THEN** cada pista de la previsualización muestra la carátula de su álbum

#### Scenario: La previsualización cruza a otro álbum
- **WHEN** el ajuste "Al terminar un álbum o artista" está en "Seguir con el siguiente" y suena la penúltima pista de un álbum
- **THEN** la previsualización muestra la última pista con la carátula de ese álbum, y la pista que sigue con la carátula del álbum siguiente

#### Scenario: Cola de carpeta
- **WHEN** el usuario reproduce desde el navegador de carpetas
- **THEN** la previsualización muestra el marcador por defecto en cada pista, igual que antes de este cambio

#### Scenario: Pista sin carátula
- **WHEN** una próxima pista de una cola de biblioteca no tiene carátula disponible
- **THEN** esa pista muestra el marcador por defecto
