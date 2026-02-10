cmake_minimum_required (VERSION 3.20)

# Project identification
project ("zwm")
find_package(X11 REQUIRED)
find_package(Freetype REQUIRED)
include(GNUInstallDirs)

set(APP_NAME "zwm")
set(APP_VERSION "alpha19")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
message (STATUS "*** Building ${APP_NAME} ${APP_VERSION} ***")

# Include files
include_directories(/usr/local/include ${X11_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS})

# Project config file
configure_file(${zwm_SOURCE_DIR}/src/version.h.in ${zwm_BINARY_DIR}/src/version.h)
add_definitions(-I ${zwm_BINARY_DIR}/src)
add_compile_options(-O2 -Wall -Wextra)

# Linker options
link_libraries(-L/usr/local/lib -lm ${X11_LIBRARIES} ${X11_Xft_LIB} ${X11_Xrandr_LIB} ${FREETYPE_LIBRARIES})

# Project sub-directories
add_subdirectory(src)

# Additional files
install(FILES man/zwm.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES examples/config DESTINATION share/examples/${APP_NAME})
