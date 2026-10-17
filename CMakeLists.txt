cmake_minimum_required(VERSION 3.21)
project(SVE VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Multimedia Svg)

add_library(sve_media STATIC src/mediaengine.cpp src/mediaengine.h)
target_link_libraries(sve_media PUBLIC Qt6::Core)
target_include_directories(sve_media PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(sve src/main.cpp src/mainwindow.cpp src/mainwindow.h src/timelinewidget.cpp src/timelinewidget.h src/videosurface.cpp src/videosurface.h assets/sve.qrc)
target_link_libraries(sve PRIVATE sve_media Qt6::Widgets Qt6::Multimedia Qt6::Svg)
target_compile_definitions(sve PRIVATE SVE_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
set(SVE_EXECUTABLE "${CMAKE_INSTALL_FULL_BINDIR}/sve")
set(SVE_ICON "${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/256x256/apps/sve.png")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/assets/sve.desktop.in"
  "${CMAKE_CURRENT_BINARY_DIR}/sve.desktop"
  @ONLY
)
install(TARGETS sve RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sve.desktop" DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/sve.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
foreach(size 48 128 256)
  install(FILES "assets/sve-${size}.png"
      DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps"
      RENAME "sve.png")
endforeach()

include(CTest)
if(BUILD_TESTING)
  add_executable(sve_backend_tests tests/backend_tests.cpp)
  target_link_libraries(sve_backend_tests PRIVATE sve_media Qt6::Core)
  add_test(NAME media_backend COMMAND sve_backend_tests)
  add_test(NAME ffmpeg_smoke COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/ffmpeg_smoke.sh")
  add_test(NAME launcher_smoke COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/launcher_smoke.sh")
  add_test(NAME preview_about_regression COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/preview_about_smoke.sh")
  add_test(NAME toolbar_about_regression COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/toolbar_about_smoke.sh")
  add_test(NAME aur_template_regression COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/aur_smoke.sh")
  add_test(NAME completion_regression COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/completion_regression.sh")
endif()
