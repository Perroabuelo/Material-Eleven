cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(ElevenMPV)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Eleven Music Player")
set(VITA_TITLEID  "ELEVENMPV")
set(VITA_VERSION  "02.10")

# -g rides along with -O3: it emits DWARF without changing the generated code,
# which is what lets vita-parse-core resolve function names and line numbers out
# of a psp2core dump. Both GPU crashes during vectorize-ui-controls were located
# by hand for want of it.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g -Wall -Werror -fcommon")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

# No PNG is linked into the binary any more: every icon and indicator is drawn
# as geometry, and cover art comes from the track being played. The machinery
# that embedded res/*.png went with them.

include_directories(
  libs/include
  include
  include/audio
  include/menus
  # FreeType answers "does this face have a glyph for this codepoint", which is
  # what the non-Latin fallback splits a string on. Our own Cyrillic coverage is
  # partial, so the question cannot be answered from ranges.
  ${VITASDK}/arm-vita-eabi/include/freetype2
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/libs/lib
)

add_executable(${PROJECT_NAME}
  source/common.c
  source/config.c
  source/dirbrowse.c
  source/fs.c
  source/main.c
  source/nav_rail.c
  source/status_bar.c
  source/touch.c
  source/ui_gpu.c
  source/ui_theme.c
  source/utils.c
  source/audio/audio.c
  source/audio/flac.c
  source/audio/mp3.c
  source/audio/ogg.c
  source/audio/opus.c
  source/audio/vitaaudiolib.c
  source/audio/wav.c
  source/audio/xm.c
  source/menus/menu_audioplayer.c
  source/menus/menu_displayfiles.c
  source/menus/menu_settings.c
)

target_link_libraries(${PROJECT_NAME}
  vita2d
  freetype
  bz2
  png
  jpeg
  z
  c
  FLAC
  opusfile
  opus
  vorbisfile
  vorbis
  ogg
  xmp-lite
  m
  mpg123
  SceAppMgr_stub
  SceAppUtil_stub
  SceAudio_stub
  SceAudiodec_stub
  SceCommonDialog_stub
  SceCtrl_stub
  SceDisplay_stub
  SceGxm_stub
  SceIme_stub
  ScePower_stub
  ScePvf_stub
  SceRtc_stub
  SceShellSvc_stub
  SceSysmem_stub
  SceSysmodule_stub
  SceTouch_stub
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  FILE res/Manrope.ttf Manrope.ttf
  FILE res/IBMPlexMono-Medium.ttf IBMPlexMono-Medium.ttf
)
