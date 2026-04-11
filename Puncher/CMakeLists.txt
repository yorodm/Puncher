cmake_minimum_required(VERSION 3.25)
project(Puncher VERSION 1.0.0)

set(IPLUG2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../iPlug2 CACHE PATH "iPlug2 root directory")
include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    EnvelopeDisplayControl.cpp
    EnvelopeDisplayControl.h
    IVolumeNiceMeter.cpp
    IVolumeNiceMeter.h
    Puncher.cpp
    Puncher.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
  FORMATS
    VST3 APP
)
