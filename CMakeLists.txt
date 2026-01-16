cmake_minimum_required(VERSION 3.18)
project(IgsMesh LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(OCCT_ROOT "" CACHE PATH "OpenCASCADE install root (contains include/ and lib/)")

if(DEFINED OpenCASCADE_DIR)
  if(EXISTS "${OpenCASCADE_DIR}/OpenCASCADEConfig.cmake")
    if(NOT EXISTS "${OpenCASCADE_DIR}/OpenCASCADEFoundationClassesTargets.cmake"
       OR NOT EXISTS "${OpenCASCADE_DIR}/OpenCASCADEModelingDataTargets.cmake"
       OR NOT EXISTS "${OpenCASCADE_DIR}/OpenCASCADEModelingAlgorithmsTargets.cmake")
      unset(OpenCASCADE_DIR CACHE)
    endif()
  endif()
endif()

if(OCCT_ROOT STREQUAL "")
  find_package(OpenCASCADE CONFIG QUIET)
else()
  set(OpenCASCADE_FOUND FALSE)
endif()

set(OCCT_INCLUDE_DIRS "")
set(OCCT_LIBS "")

if(OpenCASCADE_FOUND)
  if(DEFINED OpenCASCADE_INCLUDE_DIRS)
    set(OCCT_INCLUDE_DIRS ${OpenCASCADE_INCLUDE_DIRS})
  elseif(DEFINED OpenCASCADE_INCLUDE_DIR)
    set(OCCT_INCLUDE_DIRS ${OpenCASCADE_INCLUDE_DIR})
  endif()
  set(OCCT_LIBS ${OpenCASCADE_LIBRARIES})
else()
  if(OCCT_ROOT STREQUAL "")
    message(FATAL_ERROR "OpenCASCADE not found. Set OCCT_ROOT to your OCCT install directory.")
  endif()

  find_path(OCCT_INCLUDE_DIR
    NAMES Standard.hxx
    PATHS
      "${OCCT_ROOT}/inc"
      "${OCCT_ROOT}/include/opencascade"
      "${OCCT_ROOT}/include"
    NO_DEFAULT_PATH
  )

  if(NOT OCCT_INCLUDE_DIR)
    message(FATAL_ERROR "OpenCASCADE headers not found under OCCT_ROOT=${OCCT_ROOT}")
  endif()

  set(OCCT_INCLUDE_DIRS ${OCCT_INCLUDE_DIR})

  set(_occt_lib_paths
    "${OCCT_ROOT}/lib"
    "${OCCT_ROOT}/win64/vc17/lib"
    "${OCCT_ROOT}/win64/vc16/lib"
    "${OCCT_ROOT}/win64/vc15/lib"
    "${OCCT_ROOT}/win64/vc14/lib"
  )

  set(_occt_required_libs
    TKernel
    TKMath
    TKG2d
    TKG3d
    TKGeomBase
    TKBRep
    TKTopAlgo
    TKGeomAlgo
    TKShHealing
    TKMesh
    TKXSBase
    TKDEIGES
    TKService
    TKV3d
    TKOpenGl
  )

  foreach(libName IN LISTS _occt_required_libs)
    find_library(_found_${libName}
      NAMES ${libName} ${libName}d
      PATHS ${_occt_lib_paths}
      PATH_SUFFIXES "" lib
      NO_DEFAULT_PATH)
    if(NOT _found_${libName})
      message(FATAL_ERROR "OpenCASCADE library '${libName}' not found under OCCT_ROOT=${OCCT_ROOT}")
    endif()
    list(APPEND OCCT_LIBS ${_found_${libName}})
  endforeach()

  find_library(_found_TKIGES
    NAMES TKIGES TKIGESd
    PATHS ${_occt_lib_paths}
    PATH_SUFFIXES "" lib
    NO_DEFAULT_PATH)
  if(_found_TKIGES)
    list(APPEND OCCT_LIBS ${_found_TKIGES})
  endif()
endif()

set(QT_REQUIRED_COMPONENTS Widgets OpenGL)
find_package(Qt6 QUIET COMPONENTS ${QT_REQUIRED_COMPONENTS})
if(Qt6_FOUND)
  set(QT_PACKAGE Qt6)
else()
  find_package(Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
  set(QT_PACKAGE Qt5)
endif()

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE INC_FILES CONFIGURE_DEPENDS include/*.h)

add_executable(IgsMesh ${SRC_FILES} ${INC_FILES})

target_include_directories(IgsMesh PRIVATE include ${OCCT_INCLUDE_DIRS})

target_link_libraries(IgsMesh
  PRIVATE
    ${QT_PACKAGE}::Widgets
    ${QT_PACKAGE}::OpenGL
    ${OCCT_LIBS}
)

if(WIN32)
  add_custom_command(TARGET IgsMesh POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
      -DDEST_DIR=$<TARGET_FILE_DIR:IgsMesh>
      -DOCCT_ROOT=${OCCT_ROOT}
      -P "${CMAKE_SOURCE_DIR}/cmake/CopyOcctDlls.cmake"
    VERBATIM
  )

  set(DEPLOY_QT ON CACHE BOOL "Run windeployqt after build")
  if(DEPLOY_QT)
    set(_windeployqt "")
    if(Qt6_FOUND)
      get_target_property(_qmake_exe Qt6::qmake IMPORTED_LOCATION)
      if(_qmake_exe)
        get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
        find_program(_windeployqt NAMES windeployqt HINTS "${_qt_bin_dir}")
      endif()
    endif()
    if(NOT _windeployqt AND Qt5_FOUND)
      get_target_property(_qmake_exe Qt5::qmake IMPORTED_LOCATION)
      if(_qmake_exe)
        get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
        find_program(_windeployqt NAMES windeployqt HINTS "${_qt_bin_dir}")
      endif()
    endif()

    if(_windeployqt)
      add_custom_command(TARGET IgsMesh POST_BUILD
        COMMAND "${_windeployqt}"
          --no-translations
          --no-system-d3d-compiler
          --no-opengl-sw
          "$<TARGET_FILE:IgsMesh>"
        VERBATIM
      )
    endif()
  endif()
endif()

if(MSVC)
  target_compile_options(IgsMesh PRIVATE /W4 /permissive-)
else()
  target_compile_options(IgsMesh PRIVATE -Wall -Wextra -Wpedantic)
endif()

