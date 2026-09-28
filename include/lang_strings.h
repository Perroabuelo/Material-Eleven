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
