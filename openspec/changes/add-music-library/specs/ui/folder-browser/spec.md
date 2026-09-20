## ADDED Requirements

### Requirement: El listado de una carpeta nunca desborda su reserva
El sistema SHALL acotar el número de entradas que lee de una carpeta al tamaño de la memoria que tiene reservada para ellas. Cuando una carpeta contiene más entradas de las que esa reserva admite, el sistema SHALL mostrar las que caben y seguir funcionando con normalidad, y SHALL NOT escribir fuera de la reserva. Esto SHALL aplicar por igual a la construcción del listado y a la construcción de la cola de reproducción a partir de la carpeta.

#### Scenario: Carpeta con más entradas de las que caben
- **WHEN** el usuario entra en una carpeta que contiene más entradas de las que la aplicación puede listar
- **THEN** se muestran las entradas que caben, la navegación y la reproducción siguen respondiendo, y la aplicación no se cae ni corrompe lo que muestra

#### Scenario: Reproducir desde una carpeta con más entradas de las que caben
- **WHEN** el usuario reproduce un archivo de una carpeta que contiene más entradas de las que la aplicación puede listar
- **THEN** la reproducción comienza y la cola se construye con las pistas que caben, sin caídas
