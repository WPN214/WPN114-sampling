# Copyright (c) 2019 Pierre Cochard (alias wpn214)
# All rights reserved.

set(WPN114_AUDIO_FOUND FALSE)

# wpn214-audio & wpn214-network
find_library(WPN114_AUDIO_LIBRARY NAMES wpn114audio-qml
             HINTS /usr/lib/qt/qml/WPN114/Audio)

find_path(WPN114_AUDIO_ROOT include/wpn114audio/graph.hpp
          PATHS /usr/lib/qt/qml/WPN114/Audio)

find_path(WPN114_AUDIO_INCLUDE_DIR wpn114audio/graph.hpp
          PATHS ${WPN114_AUDIO_ROOT}/include)

if (WPN114_AUDIO_LIBRARY AND WPN114_AUDIO_INCLUDE_DIR)
    set(WPN114_AUDIO_FOUND TRUE)

  if (NOT TARGET "WPN114::Audio")
    add_library("WPN114::Audio" UNKNOWN IMPORTED)
    set_target_properties("WPN114::Audio"
      PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${WPN114_AUDIO_INCLUDE_DIR}"
      IMPORTED_LOCATION "${WPN114_AUDIO_LIBRARY}"
      INTERFACE_LINK_LIBRARIES "WPN114::Audio")
  endif()
endif()
