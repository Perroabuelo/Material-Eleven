// La tabla de textos de la interfaz. Cada linea es un texto: su id, el ingles y
// el espanol, en ese orden y en la misma linea para poder revisarlos lado a lado.
//
// No tiene guardas de inclusion a proposito: lang.h y lang.c la incluyen varias
// veces con distintas definiciones de X, y de la misma lista salen el enum y los
// dos arreglos. Como X exige los tres argumentos, un texto sin su traduccion no
// compila.
//
// Los textos con especificadores % se usan como formato de snprintf. Los dos
// idiomas tienen que llevar los mismos especificadores en el mismo orden; lo
// comprueba tests/test_lang.c.
//
// Espanol neutro: tuteo en las frases, infinitivo en los hints de botones, y
// acentos y ene completos. Las etiquetas en mayusculas se escriben ya en
// mayusculas, porque toupper no sabe de UTF-8.

// ---- Ajustes ----
X(STR_SETTINGS_TITLE,        "Settings",                 "Ajustes")
X(STR_SETTINGS_STORAGE,      "Storage",                  "Almacenamiento")
X(STR_SETTINGS_SORT,         "Sort order",               "Orden")
X(STR_SETTINGS_METADATA,     "Metadata",                 "Metadatos")
X(STR_SETTINGS_NORMALIZER,   "Normalizer",               "Normalizador")
X(STR_SETTINGS_EQUALIZER,    "Equalizer",                "Ecualizador")
X(STR_SETTINGS_LANGUAGE,     "Language",                 "Idioma")

X(STR_SORT_NAME_AZ,          "Name (A-Z)",               "Nombre (A-Z)")
X(STR_SORT_NAME_ZA,          "Name (Z-A)",               "Nombre (Z-A)")
X(STR_SORT_SIZE_DESC,        "Size (largest first)",     "Tamaño (mayor primero)")
X(STR_SORT_SIZE_ASC,         "Size (smallest first)",    "Tamaño (menor primero)")
X(STR_SORT_HINT_NAME_AZ,     "A-Z",                      "A-Z")
X(STR_SORT_HINT_NAME_ZA,     "Z-A",                      "Z-A")
X(STR_SORT_HINT_SIZE_DESC,   "Size v",                   "Tam. v")
X(STR_SORT_HINT_SIZE_ASC,    "Size ^",                   "Tam. ^")

X(STR_META_FLAC,             "FLAC metadata",            "Metadatos FLAC")
X(STR_META_MP3,              "MP3 metadata",             "Metadatos MP3")
X(STR_META_OPUS,             "OPUS metadata",            "Metadatos OPUS")

X(STR_ALC_OFF,               "Normalizer off",           "Normalizador desactivado")
X(STR_ALC_ON,                "Normalizer on",            "Normalizador activado")
X(STR_ALC_HINT_OFF,          "Off",                      "Desactivado")
X(STR_ALC_HINT_ON,           "On",                       "Activado")

X(STR_EQ_OFF,                "Off",                      "Apagado")
X(STR_EQ_LIMIT_VOLUME,       "Limit volume with EQ",     "Limitar volumen con EQ")

// "English" y "Español" no estan aqui: se muestran siempre en su propio idioma,
// para que cualquiera reconozca el suyo (menu_settings.c).
X(STR_LANG_SYSTEM,           "System",                   "Sistema")

X(STR_HINT_SELECT,           "Select",                   "Seleccionar")
X(STR_HINT_BACK,             "Back",                     "Atrás")
X(STR_HINT_SETTINGS_CATEGORY, "L . R - change category", "L . R - cambiar de categoría")

// ---- Carpetas ----
X(STR_FOLDER,                "Folder",                   "Carpeta")
X(STR_PARENT_FOLDER,         "Parent folder",            "Carpeta superior")
X(STR_FOLDERS_ONE,           "%d FOLDER",                "%d CARPETA")
X(STR_FOLDERS_MANY,          "%d FOLDERS",               "%d CARPETAS")
X(STR_TRACKS_CAPS_ONE,       "%d TRACK",                 "%d PISTA")
X(STR_TRACKS_CAPS_MANY,      "%d TRACKS",                "%d PISTAS")
X(STR_SEARCH_FOLDER,         "Search this folder",       "Buscar en esta carpeta")
X(STR_HINT_CANCEL_SEARCH,    "L + R + START - Cancel search", "L + R + START - Cancelar la búsqueda")
X(STR_HINT_ENTER,            "Enter",                    "Entrar")
X(STR_HINT_CANCEL,           "Cancel",                   "Cancelar")
X(STR_HINT_CHOOSE_FOLDER,    "Triangle - Choose this folder", "Triángulo - Elegir esta carpeta")
X(STR_HINT_OPEN_PLAY,        "Open / Play",              "Abrir / Reproducir")
X(STR_HINT_CLEAR_FILTER,     "Clear filter",             "Quitar filtro")
X(STR_HINT_SETTINGS,         "SELECT - Settings",        "SELECT - Ajustes")
X(STR_HINT_EXIT,             "START - Exit",             "START - Salir")

// ---- Biblioteca ----
X(STR_LIBRARY_TITLE,         "Library",                  "Biblioteca")
X(STR_VIEW_SONGS,            "Songs",                    "Canciones")
X(STR_VIEW_ARTISTS,          "Artists",                  "Artistas")
X(STR_VIEW_ALBUMS,           "Albums",                   "Álbumes")
X(STR_VIEW_RECENT,           "Recent",                   "Recientes")
X(STR_UNKNOWN,               "Unknown",                  "Desconocido")
X(STR_TRACKS_ONE,            "%d track",                 "%d pista")
X(STR_TRACKS_MANY,           "%d tracks",                "%d pistas")
X(STR_COUNTER,               "%d of %d",                 "%d de %d")
X(STR_COUNTER_TRUNCATED,     "%d of %d  -  library truncated at the limit", "%d de %d  -  biblioteca truncada en el máximo")
X(STR_COUNTER_PENDING_ONE,   "%d of %d  -  %d tag left to read",  "%d de %d  -  falta %d etiqueta por leer")
X(STR_COUNTER_PENDING_MANY,  "%d of %d  -  %d tags left to read", "%d de %d  -  faltan %d etiquetas por leer")
X(STR_NOTICE_TRUNCATED,      "The collection is over the limit: the library was truncated.", "La colección supera el máximo: la biblioteca quedó truncada.")
X(STR_NOTICE_ROOT_GONE,      "That folder is no longer available. Choose another with Square.", "Esa carpeta ya no está disponible. Elige otra con Cuadrado.")
X(STR_NOTICE_FILE_GONE,      "That file is gone. Rescan to bring the library up to date.", "Ese archivo ya no está. Reescanea para poner la biblioteca al día.")
X(STR_NOTICE_UNREADABLE,     "Couldn't read that file. It may be damaged or incomplete.", "No se pudo leer ese archivo. Puede estar dañado o incompleto.")
X(STR_ROOT_MISSING,          "The library folder is not available. Square - choose another.", "La carpeta de la biblioteca no está disponible. Cuadrado - elegir otra.")
X(STR_EMPTY_NO_ROOT,         "No library folder has been chosen yet.", "Todavía no hay una carpeta para la biblioteca.")
X(STR_EMPTY_UNBUILT,         "This folder hasn't been scanned yet.", "Esta carpeta todavía no se escaneó.")
X(STR_EMPTY_NO_TRACKS,       "There were no playable tracks in that folder.", "En esa carpeta no había ninguna pista reproducible.")
X(STR_ACTION_CHOOSE_FOLDER,  "Square - Choose folder",   "Cuadrado - Elegir carpeta")
X(STR_ACTION_SCAN,           "Triangle - Scan",          "Triángulo - Escanear")
X(STR_ACTION_CHOOSE_OTHER,   "Square - Choose another folder", "Cuadrado - Elegir otra carpeta")
X(STR_HINT_START,            "Start",                    "Empezar")
X(STR_HINT_OPEN,             "Open",                     "Abrir")
X(STR_HINT_PLAY,             "Play",                     "Reproducir")
X(STR_HINT_RETURN,           "Back",                     "Volver")
X(STR_HINT_VIEWS,            "L R - Views",              "L R - Vistas")
X(STR_HINT_RESCAN,           "Triangle - Rescan",        "Triángulo - Reescanear")
X(STR_HINT_FOLDER,           "Square - Folder",          "Cuadrado - Carpeta")
