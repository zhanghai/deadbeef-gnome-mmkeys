cmake_minimum_required(VERSION 3.13)
project(deadbeef-gnome-mmkeys C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)
find_package(GLIB REQUIRED gio)

add_library(ddb_gnome_mmkeys SHARED ddb_gnome_mmkeys.c)
target_include_directories(ddb_gnome_mmkeys PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(ddb_gnome_mmkeys PRIVATE ${GLIB_LIBRARIES} ${GLIB_GIO_LIBRARIES})
