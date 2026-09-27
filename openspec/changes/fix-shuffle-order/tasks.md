## 1. Etapa 1 — La cola gana el orden, con la identidad puesta

- [x] 1.1 Añadir el arreglo de orden paralelo a rutas y títulos, y hacer que `Queue_Grow` reserve los tres antes de publicar la nueva capacidad, sin publicarla si alguno falla; verificar reproduciendo una carpeta con más pistas que la reserva inicial que la cola crece sin perder ni desalinear ninguna
- [x] 1.2 Implementar el orden identidad y, sobre él, `Queue_Advance`, `Queue_SeekToPath` y `Queue_PeekAhead`, conviviendo todavía con la posición actual; verificar que reproducir una carpeta entera de principio a fin suena exactamente en el mismo orden que antes del change
- [x] 1.3 Implementar `Queue_SetShuffle` y `Queue_IsShuffled` con Fisher-Yates sobre el rango y la pista en curso clavada en el slot 0, sin que ninguna superficie lo encienda todavía; verificar recorriendo la cola entera que cada pista aparece exactamente una vez y que ninguna queda fuera
- [x] 1.4 Verificar que encender y apagar el barajado sobre una cola vacía y sobre una cola de una sola pista no rompe el avance ni la previsualización

## 2. Etapa 2 — El reproductor pasa a la API nueva

- [x] 2.1 Reescribir `Music_HandleNext` para que solo avance y abra, pidiendo la ruta a la cola en vez de sortear; verificar que el avance al terminar una pista se comporta igual que hoy con el barajado apagado
- [x] 2.2 Pasar `Menu_InitMusic` a `Queue_SeekToPath` y retirar `Queue_GetPosition` y `Queue_SetPosition` del header; verificar que el proyecto compila sin ellas y que reproducir desde una carpeta y desde una vista de biblioteca sigue colocando la cola en la pista correcta
- [x] 2.3 Hacer que los cuatro caminos de avance —transporte táctil, gatillos L/R, `Music_Next`/`Music_Previous` del mini reproductor y el fin natural de la pista— recorran el orden vigente; verificar en consola con el barajado encendido que los cuatro saltan a la pista del plan, que es el defecto reportado
- [x] 2.4 Pasar el bucle de rescate de pistas que no abren a caminar en slots; verificar borrando un archivo de la cola que se salta y la reproducción continúa por el plan, y provocando que ninguna abra que el sistema sale de la pantalla de reproducción sin colgarse ni quedarse sin pantalla
- [x] 2.5 Retirar el `Queue_SetPosition` de `source/menus/menu_library.c:371`, que queda cubierto por el salto a ruta; verificar que reproducir desde una vista de biblioteca arranca por la pista que el usuario tocó

## 3. Etapa 3 — Barajado y repetición como banderas independientes

- [x] 3.1 Partir el `int state` de tres valores en dos banderas independientes y conectar los dos toggles táctiles y los botones Triángulo y Cuadrado; verificar en consola que encender una no apaga la otra y que ambos glifos pueden aparecer activos a la vez
- [x] 3.2 Reescribir el bloque de fin de pista para que la repetición reabra la misma pista y, cuando está apagada, se avance por el orden vigente; verificar en consola las cuatro combinaciones de las dos banderas
- [x] 3.3 Verificar que con la repetición y el barajado encendidos, pedir la pista siguiente a mano avanza por el plan barajado en vez de repetir la actual

## 4. Etapa 4 — La previsualización dice la verdad

- [x] 4.1 Pasar `Menu_DrawUpNext` a `Queue_PeekAhead`; verificar que con el barajado apagado muestra las mismas pistas que mostraba antes del change
- [x] 4.2 Verificar en consola que encender el barajado a mitad de una pista cambia la lista de próximas en el acto, sin esperar a que la pista en curso termine
- [x] 4.3 Retirar el mensaje de final de cola y mostrar en su lugar las pistas por las que la reproducción va a continuar; verificar en la última pista de una carpeta que anuncia la primera, y con el barajado encendido que anuncia la primera del plan

## 5. Cierre

- [x] 5.1 Ejecutar `openspec validate fix-shuffle-order --strict` y verificar que pasa
- [x] 5.2 Verificar en consola un recorrido completo sobre una colección real: reproducir desde carpeta y desde vista de biblioteca, encender y apagar el barajado a mitad de pista, avanzar y retroceder desde el transporte, desde los gatillos y desde el mini reproductor, recorrer una cola hasta el final y comprobar por dónde continúa
- [x] 5.3 Verificar que el navegador de carpetas y la biblioteca se comportan igual que antes del change en todo lo que no sea el orden de reproducción, y que al reiniciar la aplicación el barajado y la repetición nacen apagados
