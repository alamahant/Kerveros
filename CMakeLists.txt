cmake_minimum_required(VERSION 3.16)

project(Kerveros VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(FLATHUB_BUILD "Build for Flathub/Flatpak" OFF)

if(FLATHUB_BUILD)
  # Set source directories (these will exist in the Flatpak build sandbox)
  set(IMAGEMAGICK_SRC_DIR "${CMAKE_SOURCE_DIR}/imagemagick_src")
  set(ZBAR_SRC_DIR "${CMAKE_SOURCE_DIR}/zbar_src")


  # Check if source directories exist
  if(NOT EXISTS ${IMAGEMAGICK_SRC_DIR})
      message(FATAL_ERROR "ImageMagick source directory not found at ${IMAGEMAGICK_SRC_DIR}")
    endif()
  if(NOT EXISTS ${ZBAR_SRC_DIR})
    message(FATAL_ERROR "zbar source directory not found at ${ZBAR_SRC_DIR}")
  endif()

  # Build ImageMagick first
  execute_process(
    COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX}
    WORKING_DIRECTORY ${IMAGEMAGICK_SRC_DIR}
    RESULT_VARIABLE IMAGEMAGICK_CONFIGURE_RESULT
  )
  if(NOT IMAGEMAGICK_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to configure ImageMagick")
  endif()

  execute_process(
    COMMAND make
    WORKING_DIRECTORY ${IMAGEMAGICK_SRC_DIR}
    RESULT_VARIABLE IMAGEMAGICK_MAKE_RESULT
  )
  if(NOT IMAGEMAGICK_MAKE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to build ImageMagick")
  endif()

  execute_process(
    COMMAND make install
    WORKING_DIRECTORY ${IMAGEMAGICK_SRC_DIR}
    RESULT_VARIABLE IMAGEMAGICK_INSTALL_RESULT
  )
  if(NOT IMAGEMAGICK_INSTALL_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install ImageMagick")
  endif()
  # Build ZBAR
  execute_process(
    COMMAND autoreconf -vfi
    WORKING_DIRECTORY ${ZBAR_SRC_DIR}
    RESULT_VARIABLE ZBAR_AUTORECONF_RESULT
  )
  if(NOT ZBAR_AUTORECONF_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to run autoreconf for zbar")
  endif()

  execute_process(
    COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX}
    WORKING_DIRECTORY ${ZBAR_SRC_DIR}
    RESULT_VARIABLE ZBAR_CONFIGURE_RESULT
  )
  if(NOT ZBAR_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to configure zbar")
  endif()

  execute_process(
    COMMAND make
    WORKING_DIRECTORY ${ZBAR_SRC_DIR}
    RESULT_VARIABLE ZBAR_MAKE_RESULT
  )
  if(NOT ZBAR_MAKE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to build zbar")
  endif()

  execute_process(
    COMMAND make install
    WORKING_DIRECTORY ${ZBAR_SRC_DIR}
    RESULT_VARIABLE ZBAR_INSTALL_RESULT
  )
  if(NOT ZBAR_INSTALL_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install zbar")
  endif()


  # Set library paths for your project
  #set(ZBAR_LIBRARY "${CMAKE_INSTALL_PREFIX}/lib/libzbar.so" CACHE PATH "Path to zbar library")

endif()


find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
        main.cpp
        twofamanager.cpp
        twofamanager.h
        simple2fa.cpp
        simple2fa.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Kerveros
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        simple2fa.cpp simple2fa.h
        securitymanager.h
        securitymanager.cpp
        resources.qrc
        helpmenudialog.cpp helpmenudialog.h
        donationdialog.cpp donationdialog.h
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Kerveros APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Kerveros SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Kerveros
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Kerveros PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt6::Network)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Kerveros)
endif()
set_target_properties(Kerveros PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Kerveros
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

#if(FLATHUB_BUILD)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Kerveros.desktop"
          DESTINATION "share/applications")

  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/io.github.alamahant.Kerveros.metainfo.xml"
          DESTINATION "share/metainfo")

  # Install icons for different sizes
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicon_io/android-chrome-192x192.png"
          DESTINATION "share/icons/hicolor/192x192/apps"
          RENAME "io.github.alamahant.Kerveros.png")

  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicon_io/android-chrome-512x512.png"
          DESTINATION "share/icons/hicolor/512x512/apps"
          RENAME "io.github.alamahant.Kerveros.png")

  # Optionally, install 16x16 and 32x32 icons if desired
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicon_io/favicon-16x16.png"
          DESTINATION "share/icons/hicolor/16x16/apps"
          RENAME "io.github.alamahant.Kerveros.png")

  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/favicon_io/favicon-32x32.png"
          DESTINATION "share/icons/hicolor/32x32/apps"
          RENAME "io.github.alamahant.Kerveros.png")
#endif()



if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Kerveros)
endif()
