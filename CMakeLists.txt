cmake_minimum_required (VERSION 3.20)
project ("zwm" VERSION 0.19)
find_package(X11 REQUIRED)
find_package(Freetype REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-O2 -Wall -Wextra)

include_directories(${X11_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS})
include_directories(${CMAKE_BINARY_DIR}/src)

link_libraries(-lm ${X11_LIBRARIES} ${X11_Xft_LIB})
link_libraries(${X11_Xrandr_LIB} ${FREETYPE_LIBRARIES})

configure_file(${CMAKE_SOURCE_DIR}/src/version.h.in ${CMAKE_BINARY_DIR}/src/version.h)
add_subdirectory(src)

include(GNUInstallDirs)
install(FILES man/zwm.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES examples/config DESTINATION share/examples/${APP_NAME})
message (STATUS "<<< ${PROJECT_NAME} version ${PROJECT_VERSION} is configured. >>>")
