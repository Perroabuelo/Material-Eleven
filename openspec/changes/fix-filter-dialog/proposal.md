## Why

El buscador de la pantalla de carpetas nunca ha funcionado. Al tocar la píldora "Buscar en esta carpeta" la pantalla queda en un color plano, no aparece ningún teclado, no responde ningún botón y la única salida es cerrar la aplicación con el botón PS. La causa está identificada y es única: el lazo que atiende el diálogo de teclado del sistema pide la actualización del diálogo **dentro de la escena de dibujo abierta**, de modo que el diálogo nunca se compone ni avanza de estado, el lazo gira indefinidamente y se queda con el control de la aplicación.

Se descartó el multisampling como causa: fue introducido después del buscador (`0409e82` frente a `472c2c9`) y el usuario confirma que el buscador nunca llegó a funcionar, ni antes ni después de ese commit.

## What Changes

- **Componer el diálogo del sistema fuera de la escena de dibujo.** La actualización del diálogo pasa a ocurrir una vez cerrada la escena del fotograma y antes de presentar el buffer, que es el único punto donde el sistema puede componerlo. Este es el defecto; todo lo demás de este change existe porque el defecto dejó ver que la ruta no tenía ninguna red.
- **Ningún lazo de diálogo sin salida.** El lazo actual es `while (estado == EN_EJECUCIÓN)` sin cancelación ni techo: cualquier fallo futuro del diálogo vuelve a bloquear la aplicación entera. Pasa a tener una salida garantizada que devuelve el control a la pantalla anterior dejando el filtro como estaba.
- **Configurar el subsistema de diálogos comunes al arrancar.** Hoy no se le comunica el idioma del sistema ni qué botón es "aceptar", pese a que la aplicación ya conoce ambos datos. El diálogo pasa a respetar la preferencia de botones de la consola, igual que el resto de la interfaz.
- **El lazo del diálogo dibuja la pantalla, no un color plano.** El lazo limpiaba el fotograma y no dibujaba nada más; eso es exactamente el "color por default" observado. Pasa a dibujarse la pantalla que el usuario tenía delante, con la salida anunciada en la leyenda de botones. *Comprobado después en consola: con el teclado ya compuesto no se ve nada de eso, porque el diálogo del sistema cubre la pantalla y no se consiguió que dejara de hacerlo (ver `tasks.md`, 3.1). El redibujado se mantiene porque es lo que queda en pantalla cuando el diálogo no compone, que es justamente el caso reportado, y el requisito de ver la lista detrás se retiró del delta de spec.*
- **Cerrar el segundo bloqueo de la misma función.** Un filtro sin coincidencias en la raíz del dispositivo deja la lista vacía, y como la acción de cancelar solo está disponible por debajo de la raíz, no queda ningún botón físico que permita salir de ese estado: el filtro solo puede quitarse por toque. El botón de cancelar pasa a limpiar el filtro cuando hay uno activo.
- **Entrega por partes.** Los commits se realizan por etapas, nunca agrupando el change en un commit único, y cada etapa deja el árbol compilable y verificado en hardware antes de empezar la siguiente. Es la misma regla de entrega que `close-ui-contract`, y aquí importa especialmente porque el defecto solo se puede confirmar en consola.

Fuera de alcance, confirmado con el usuario: la biblioteca de música con agrupación por artista y álbum, la lectura de tags en un índice cacheado, las playlists, y ocultar de la lista los archivos que no son audio. Todo eso se exploró junto a este defecto y se aparta deliberadamente a un change posterior, para que la corrección del bloqueo no quede detenida detrás de un trabajo mucho mayor.

## Capabilities

### New Capabilities

Ninguna. Las dos superficies que este change toca ya están cubiertas por capabilities vigentes.

### Modified Capabilities

- `ui/folder-browser`: el requirement del filtro en carpeta describe hoy únicamente el efecto de filtrar ("el usuario escribe un término y la lista se reduce"), sin decir nada de cómo se introduce ese término ni de cómo se sale de esa introducción. Esa omisión es precisamente lo que permitió que la única ruta de entrada al filtro quedara rota y bloqueante sin violar el spec. Pasa a exigir que la entrada de texto se presente, que siempre pueda abandonarse, y que el usuario nunca quede sin una salida por botón físico cuando el filtro deja la lista vacía.
- `ui/rendering`: la capability ya fija invariantes de ciclo de vida de recursos gráficos y de degradación observable, pero no dice nada sobre los diálogos del sistema ni sobre la disciplina del lazo de fotogramas. Se le añade el invariante que faltaba: un diálogo del sistema se compone fuera de la escena de dibujo del fotograma, y ninguna operación que retenga el lazo de fotogramas puede carecer de salida.

## Impact

- **Código**: `source/menus/menu_displayfiles.c` es el archivo central del change — `Menu_PromptFilter` (orden de composición, salida del lazo, fondo dibujado) y `Menu_HandleControls` (el botón de cancelar limpiando el filtro). `source/main.c` suma la configuración del subsistema de diálogos comunes junto al resto de la inicialización de servicios del sistema.
- **Sin cambios**: `source/dirbrowse.c` e `include/dirbrowse.h` ya exponen lo necesario para limpiar y consultar el filtro; este change no toca la lógica de filtrado en sí, que funciona.
- **Sin dependencias nuevas**: el módulo de IME ya se carga y se descarga en el arranque y la salida.
- **Verificación**: el defecto es de comportamiento del sistema y no se puede reproducir ni confirmar fuera de la consola. Cada etapa se prueba en hardware, y la prueba de aceptación mínima es abrir el buscador, escribir un término, aceptarlo, volver a abrirlo, cancelarlo, y comprobar que la aplicación sigue respondiendo en los cuatro casos.
- **Riesgo residual**: si tras corregir el orden de composición el diálogo siguiera sin aparecer, el siguiente sospechoso es el modo de multisampling establecido al arrancar. La salida garantizada del lazo hace que ese escenario deje la aplicación utilizable en vez de bloqueada, de modo que el diagnóstico se pueda continuar sin perder la sesión.
