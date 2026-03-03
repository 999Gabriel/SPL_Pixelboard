cmake_minimum_required(VERSION 3.16)
project(Pixelboard)

# PlatformIO Integration
include(CMakeListsPrivate.txt)

# Optionale zusätzliche Definitionen für bessere IDE-Unterstützung
add_definitions(-DARDUINO=10819)
add_definitions(-DESP32)

# Include-Pfade für bessere Code-Completion
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Button
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Joystick
)
