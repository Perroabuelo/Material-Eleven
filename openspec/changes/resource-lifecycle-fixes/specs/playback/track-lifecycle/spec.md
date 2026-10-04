## Purpose

Define qué se garantiza sobre la memoria y los recursos del sistema que usa una pista: al abrirla, al cambiar a otra y cuando no se puede abrir. Así una sesión larga de escucha no va perdiendo memoria.

## ADDED Requirements

### Requirement: Cambiar de pista no acumula memoria
Al pasar de una pista a otra, por cualquier camino (R y L en Reproduciendo, el mini reproductor, el fin de una pista, o elegir otra canción en Carpetas o en la Biblioteca), el sistema SHALL devolver toda la memoria y todos los recursos que tomó la pista anterior, incluido su hilo de audio, en cualquiera de los formatos que reproduce (FLAC, MP3, OGG, Opus, WAV y los módulos MOD, XM, IT y S3M). El overlay de debug SHALL mostrar el heap en uso para poder comprobarlo en la consola.

#### Scenario: Cincuenta cambios de pista no hacen crecer el heap
- **WHEN** el usuario abre el overlay de debug, reproduce una carpeta con pistas de todos los formatos, anota el heap en uso, pasa de pista con R cincuenta veces y vuelve a la primera pista
- **THEN** el heap en uso que muestra el overlay es el mismo que el anotado, con una diferencia de no más de unos pocos KB

#### Scenario: Las pistas de tracker no pierden memoria
- **WHEN** el usuario, con el overlay de debug abierto, reproduce una carpeta que solo tiene módulos MOD, XM, IT o S3M, anota el heap en uso y recorre la cola completa tres veces con R
- **THEN** al volver a la primera pista, el heap en uso es el mismo que el anotado

#### Scenario: Cambiar de pista rápido no deja dos pistas sonando
- **WHEN** el usuario pulsa R veinte veces lo más rápido que puede en una carpeta de FLAC y espera a que la música se estabilice
- **THEN** suena una sola pista, sin cortes ni sonidos superpuestos, y el heap y la memoria libre de usuario del overlay son los mismos que antes de empezar

### Requirement: Un archivo que no abre no deja recursos tomados
Cuando el sistema intenta abrir una pista y no puede (archivo dañado, truncado o con extensión reconocida pero contenido de otro tipo), SHALL devolver todo lo que haya tomado en el intento antes de pasar a la pista siguiente o de avisar al usuario.

#### Scenario: Una carpeta con archivos dañados no pierde memoria
- **WHEN** el usuario prepara una carpeta con una pista buena y varios archivos `.mp3`, `.ogg`, `.opus` y `.xm` dañados (por ejemplo, archivos de texto renombrados), abre el overlay de debug, reproduce la pista buena, anota el heap en uso y pulsa R veinte veces
- **THEN** la pista buena vuelve a sonar en cada vuelta, y el heap en uso es el mismo que el anotado

#### Scenario: La biblioteca avisa sin perder memoria
- **WHEN** el usuario, con el overlay de debug abierto, elige en la Biblioteca un archivo dañado diez veces seguidas
- **THEN** cada vez aparece el aviso de que no se puede reproducir, y el heap en uso no cambia entre el primer intento y el décimo

### Requirement: El nombre del archivo se muestra tal cual
Cuando Reproduciendo muestra el nombre del archivo en lugar del título (porque la pista no tiene etiquetas), el sistema SHALL mostrarlo exactamente como se llama el archivo, sin interpretar ningún carácter del nombre.

#### Scenario: Un nombre con porcentaje
- **WHEN** el usuario reproduce un archivo WAV sin etiquetas llamado `100% pure %s %d.wav`
- **THEN** Reproduciendo muestra `100% pure %s %d.wav` como título, sin caracteres de más ni de menos
