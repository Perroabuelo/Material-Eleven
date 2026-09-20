> **Regla de entrega, válida para todo el change: los commits se realizan por partes.**
> Cada grupo numerado de abajo es una etapa y se entrega en sus propios commits, nunca
> agrupando varias etapas en un commit único, y dentro de una etapa se prefiere un commit
> por unidad coherente de trabajo antes que un commit grande al final. Cada etapa deja el
> árbol compilable y funcional, y **no se inicia la etapa siguiente hasta que la actual pasó
> su prueba en hardware**. El defecto de este change no se reproduce ni se confirma fuera de
> la consola, así que la bisección sobre commits pequeños es la única herramienta de
> diagnóstico real si algo sale mal.

> **Protocolo de Prueba del Buscador (PPB).** Varias tareas lo citan por nombre. Consiste en:
> abrir el buscador desde la lista de carpetas; escribir un término y aceptarlo; reabrirlo y
> cancelarlo; abrirlo y abandonarlo sin tocar nada; repetir las cuatro cosas con un track
> sonando en segundo plano; y repetirlas una vez en la raíz del dispositivo y otra en una
> carpeta anidada. Se considera superado si en los cuatro casos la aplicación vuelve a la
> lista y sigue respondiendo a botones y toque, y si la reproducción no se interrumpe.

> **Orden deliberado.** La salida garantizada va antes que la corrección de la composición,
> para que la etapa que toca el diálogo ya se pruebe con red. Ver `design.md` — Risks.

## 1. Salida garantizada del lazo del diálogo

- [ ] 1.1 Comprobar el código de error que devuelve la composición del diálogo y abandonar el lazo tras varios fotogramas consecutivos con error, que es el detector principal; verificar en consola que al abrir el buscador —que en esta etapa sigue sin componerse— la aplicación vuelve sola a la lista casi de inmediato en vez de quedar retenida
- [x] 1.2 Leer el pad dentro del lazo del diálogo, que hoy no lo hace, y añadir el abandono explícito por combinación de botones mantenida varios fotogramas; verificar en consola que la combinación devuelve a la lista y la aplicación vuelve a responder a botones y toque
- [x] 1.3 Elegir la combinación entre las que el teclado del sistema no utiliza y comprobar en consola que no se dispara por una pulsación accidental ni interfiere con el uso normal de la pantalla de carpetas
- [ ] 1.4 Añadir el techo absoluto de fotogramas como última red; verificar que, sin tocar nada y con los dos mecanismos anteriores neutralizados, la aplicación vuelve sola a la lista, y medir el tiempo real que tarda para confirmar por escrito que es holgado frente a lo que tarda una persona en escribir un término
- [x] 1.5 Comprobar que el diálogo se cierra por la vía ordenada en las tres salidas —se le pide abortar y se espera de forma acotada— y que el filtro queda exactamente como estaba antes de abrirlo: sin filtro si no lo había, con el término previo si lo había
- [x] 1.6 Cerrar la etapa: confirmar que ya no queda ninguna ruta del buscador que obligue a cerrar la aplicación desde el sistema, que es el bloqueo reportado

## 2. Composición del diálogo fuera de la escena de dibujo

- [x] 2.1 Mover la actualización del diálogo detrás del cierre de la escena del fotograma y delante de la presentación del buffer; verificar en consola que el teclado del sistema aparece efectivamente en pantalla, que es la corrección del defecto
- [x] 2.2 Escribir un término y aceptarlo; verificar que la lista se reduce a las entradas cuyo nombre lo contiene y que la píldora del buscador muestra el término vigente
- [x] 2.3 Reabrir el buscador con un filtro ya aplicado; verificar que el teclado se abre con el término vigente ya cargado, y que cancelar devuelve a la lista con ese filtro intacto
- [x] 2.4 Añadir la configuración del subsistema de diálogos comunes en el arranque, junto al resto de la inicialización de servicios del sistema; verificar que los botones que confirman y descartan el teclado coinciden con los que la aplicación usa para abrir y para volver, probándolo con las dos asignaciones que la consola permite elegir
- [x] 2.5 **No aplicó.** La tarea estaba condicionada a que el teclado siguiera sin aparecer tras 2.1, y apareció. El modo de multisampling queda descartado como causa sin necesidad de forzar la degradación, y con él el único sospechoso alternativo que `design.md` anotaba
- [x] 2.6 Ejecutar el PPB sobre esta etapa

## 3. Lo que se dibuja mientras el teclado está delante

- [x] 3.1 **No alcanzable, comprobado en consola.** El teclado del sistema cubre la pantalla y no se consiguió que dejara de hacerlo: el fondo transparente (`bgColor` a cero) se acepta y aun así no se ve nada debajo, y un atenuador propio se rechaza con `SCE_COMMON_DIALOG_ERROR_INVALID_DIMMER_COLOR`, de modo que lo que cubre es el atenuador del propio diálogo y los valores que admitiría para uno ajeno quedaron sin averiguar. El requisito correspondiente se retiró del delta de `ui/folder-browser`. El redibujado de la carpeta se mantiene igualmente, porque es lo único que queda en pantalla cuando el diálogo NO compone, que es el caso que originó este change
- [ ] 3.2 Confirmar que mientras el diálogo está abierto se dibuja pero no se procesa entrada de pantalla: con un track sonando, tocar sobre la posición del mini-reproductor y de la barra de navegación y verificar que ni la reproducción ni la pantalla activa cambian
- [x] 3.3 **No aplicable.** Verificaba que el mini-reproductor siguiera animándose detrás del teclado, y detrás del teclado no se ve nada. La mitad que sí importa —que la reproducción no se interrumpa al abrir ni al cerrar el buscador— la cubre el PPB, que se ejecuta con un track sonando
- [ ] 3.4 Ejecutar el PPB sobre esta etapa

## 4. Cerrar el segundo bloqueo: salir de un filtro sin coincidencias

- [x] 4.1 Hacer que el botón de cancelar limpie el filtro cuando hay uno activo, con precedencia sobre subir a la carpeta superior; verificar en la raíz del dispositivo aplicando un término que no coincida con nada, de modo que la lista quede vacía, y comprobando que el botón recupera la lista completa
- [x] 4.2 Verificar la precedencia por debajo de la raíz: con un filtro aplicado en una carpeta anidada, la primera pulsación quita el filtro y deja al usuario en la misma carpeta, y solo la segunda sube a la carpeta superior
- [x] 4.3 Verificar que sin filtro aplicado el botón se comporta exactamente como antes de este change, tanto en la raíz como por debajo de ella
- [x] 4.4 Actualizar la leyenda de botones en pantalla para que anuncie la acción vigente; verificar que el texto cambia al aplicar y al quitar el filtro, y que en la raíz sin filtro no anuncia una acción que no existe
- [x] 4.5 Ejecutar el PPB sobre esta etapa

## 5. Cierre del change

- [ ] 5.1 Ejecutar el PPB completo sobre el árbol final, en frío y con la consola recién arrancada
- [ ] 5.2 Recorrer uno por uno los escenarios de los dos deltas de spec (`ui/folder-browser` y `ui/rendering`) sobre la consola y confirmar que cada uno se cumple, dejando constancia de cuál se verificó con qué maniobra
- [x] 5.3 Confirmar por inspección que la lógica de filtrado en `source/dirbrowse.c` no fue modificada, que es lo que `design.md` declara fuera de alcance
- [x] 5.4 Confirmar que el historial del change son commits por etapas y no un commit único, de modo que cada etapa siga siendo bisectable
