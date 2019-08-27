cmake_minimum_required(VERSION 3.1)

set(WPN114_ADDON Sampling)
# -------------------------------------------------------------------------------------------------

string(TOLOWER ${WPN114_ADDON} WPN114_ADDON_LOWER)
string(CONCAT WPN114_PROJECT_NAME wpn114 ${WPN114_ADDON_LOWER})

project(${WPN114_PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(Qt5 REQUIRED COMPONENTS Quick Core Qml)
find_package(WPN114 REQUIRED COMPONENTS Audio)

# SOURCES -----------------------------------------------------------------------------------------

set(WPN114_ADDON_SOURCE_DIR source)
set(WPN114_ADDON_QML_DIR qml)
set(WPN114_ADDON_SOURCES
    ${WPN114_ADDON_SOURCE_DIR}/sampler.hpp
    ${WPN114_ADDON_QML_DIR}/qmldir
    ${WPN114_ADDON_QML_DIR}/${WPN114_ADDON_LOWER}.qmltypes
    qml_plugin.cpp)

# PROJECT -----------------------------------------------------------------------------------------

add_library(${PROJECT_NAME} SHARED ${WPN114_ADDON_SOURCES})
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Quick Qt5::Qml WPN114::Audio)

# INSTALL -----------------------------------------------------------------------------------------
message("[WPN114] Library will be installed in ${WPN114_AUDIO_ROOT}/${WPN114_ADDON}")

install(TARGETS ${PROJECT_NAME} DESTINATION ${WPN114_AUDIO_ROOT}/${WPN114_ADDON})
install(FILES qml/${WPN114_ADDON_LOWER}.qmltypes qml/qmldir DESTINATION ${WPN114_AUDIO_ROOT}/${WPN114_ADDON})
