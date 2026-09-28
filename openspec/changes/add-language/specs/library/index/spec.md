## MODIFIED Requirements

### Requirement: La ausencia de artista o de álbum se agrupa bajo "Desconocido"
El sistema SHALL agrupar en una entrada única todas las pistas sin artista, y en otra entrada única todas las pistas sin álbum. Esa entrada SHALL mostrarse con la etiqueta del idioma activo ("Desconocido" en español, "Unknown" en inglés), y SHALL ordenarse siempre al final de su lista, con independencia del criterio de orden vigente. Cambiar de idioma SHALL cambiar la etiqueta sin obligar a reescanear, y un artista o un álbum que de verdad se llame "Desconocido" o "Unknown" SHALL seguir siendo una entrada aparte.

#### Scenario: Pistas sin artista
- **WHEN** la biblioteca contiene pistas cuyo formato no provee artista, o cuyos tags no lo traen
- **THEN** todas ellas quedan reunidas bajo una sola entrada "Desconocido" en la vista de artistas, o "Unknown" si la interfaz está en inglés

#### Scenario: "Desconocido" no se intercala entre los nombres reales
- **WHEN** el usuario mira la vista de artistas o la de álbumes y existe la entrada "Desconocido"
- **THEN** esa entrada aparece al final de la lista, después de todos los nombres reales

#### Scenario: Cambiar de idioma con la biblioteca ya escaneada
- **WHEN** la biblioteca ya está escaneada y tiene pistas sin artista, y el usuario cambia el idioma en Ajustes y vuelve a la vista de artistas
- **THEN** la entrada aparece con la etiqueta del idioma nuevo, al final de la lista, con las mismas pistas, y sin que se haya iniciado un escaneo
