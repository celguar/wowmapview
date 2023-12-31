cmake_minimum_required(VERSION 3.24)
project(wowmapview)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_executable(wowmapview wowmapview.cpp areadb.cpp dbcfile.cpp font.cpp frustum.cpp liquid.cpp maptile.cpp menu.cpp model.cpp mpq_libmpq.cpp particle.cpp shaders.cpp sky.cpp test.cpp video.cpp wmo.cpp world.cpp)
if(WIN32)
    #set(ZLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/zlib/")
    #set(LIBMPQ_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libmpq")
    set(SDL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/SDL/include")
    #set(SDL_LIBRARY "${CMAKE_SOURCE_DIR}/SDL/lib/")
    set(SDL_LIBRARY "SDL")
    #set(SDL_MAIN_LIBRARY "${CMAKE_SOURCE_DIR}/SDL/lib/SDLmain.lib")
endif()
#target_include_directories(wowmapview PUBLIC "D:/Soft/web-scrap/vcpkg/installed/x86-windows/include")
#[[file(GLOB zlibfiles zlib/*.c libmpq/*.h *.h)

add_library(zliblocal STATIC
        ${zlibfiles}
        )]]

#target_link_libraries(libmpq PRIVATE zliblocal)
#include_directories(zlib)

set(ZLIB_PUBLIC_HDRS
        ${CMAKE_SOURCE_DIR}/zlib/zconf.h
        ${CMAKE_SOURCE_DIR}/zlib/zlib.h
        )
set(ZLIB_PRIVATE_HDRS
        ${CMAKE_SOURCE_DIR}/zlib/crc32.h
        ${CMAKE_SOURCE_DIR}/zlib/deflate.h
        ${CMAKE_SOURCE_DIR}/zlib/inffast.h
        ${CMAKE_SOURCE_DIR}/zlib/inffixed.h
        ${CMAKE_SOURCE_DIR}/zlib/inflate.h
        ${CMAKE_SOURCE_DIR}/zlib/inftrees.h
        ${CMAKE_SOURCE_DIR}/zlib/trees.h
        ${CMAKE_SOURCE_DIR}/zlib/zutil.h
        )
file(GLOB zlibfiles zlib/*.c)
set(ZLIB_SRCS
        ${zlibfiles}
        )

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/zlib/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*"
        "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})
#set(ZLIB_FULL_VERSION ${ZLIB_FULL_VERSION} PARENT_SCOPE)

add_library(zlib SHARED ${ZLIB_SRCS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS})
set_target_properties(zlib PROPERTIES DEFINE_SYMBOL ZLIB_DLL)
set_target_properties(zlib PROPERTIES SOVERSION 1)
# Creates zlib1.dll when building shared library version
#set_target_properties(zlib PROPERTIES SUFFIX "1.dll")

install(TARGETS zlib
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" )

find_package(SDL REQUIRED)
#[[add_library(sdl)
set_target_properties(SDL::SDL PROPERTIES DEFINE_SYMBOL SDL_DLL)
set_target_properties(SDL::SDL PROPERTIES SOVERSION 1)
# Creates zlib1.dll when building shared library version
set_target_properties(SDL::SDL PROPERTIES SUFFIX "1.dll")

install(TARGETS sdl
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}" )]]

#add_subdirectory(zlib)
#add_subdirectory(SDL/include)
#add_subdirectory(libmpq)

file(GLOB mpqfiles libmpq/*.cpp libmpq/*.h *.h)

add_library(libmpq STATIC
        ${mpqfiles}
        )

target_include_directories(libmpq PRIVATE
        zlib)
target_link_libraries(libmpq PRIVATE
        zlib)
#find_package(ZLIB REQUIRED)
#include_directories(${SDL_INCLUDE_DIR})
find_package(OpenGL REQUIRED)
target_link_libraries(wowmapview PRIVATE
        #ZLIB::ZLIB
        #SDL
        ${CMAKE_SOURCE_DIR}/SDL/lib/SDL.lib
        ${CMAKE_SOURCE_DIR}/SDL/lib/SDLmain.lib
        zlib
        libmpq)
target_include_directories(wowmapview PRIVATE
        ${SDL_INCLUDE_DIR}
        zlib
        #[[${ZLIB_INCLUDE_DIRS}]]
        libmpq)

#include_directories(${OPENGL_INCLUDE_DIR})
#include_directories(${ZLIB_INCLUDE_DIR})
#set(LIBMPQ_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libmpq")

#add_dependencies(wowmapview SDL::SDL)

#file(GLOB zlibfiles zlib/*.c libmpq/*.h *.h)

#add_library(zlib STATIC
#        ${zlibfiles}
#        )

#target_include_directories(wowmapview PUBLIC ${ZLIB_INCLUDE_DIR})
#target_include_directories(wowmapview PUBLIC ${LIBMPQ_INCLUDE_DIR})
#target_include_directories(wowmapview PUBLIC ${SDL_INCLUDE_DIR})
#target_link_libraries(wowmapview PRIVATE SDL)
#target_link_libraries(wowmapview
#        ${SDL_LIBRARY}
#        ${SDL_MAIN_LIBRARY})

#target_link_libraries(wowmapview PUBLIC ${SDL_MAIN_LIBRARY})

# Add conf file to dev folder
add_custom_command(
        TARGET ${PROJECT_NAME}  POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/fonts/arial.info
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/arial.info)
add_custom_command(
        TARGET ${PROJECT_NAME}  POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/fonts/arial.tga
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/arial.tga)
add_custom_command(
        TARGET ${PROJECT_NAME}  POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/SDL/lib/SDL.dll
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/SDL.dll)
file(WRITE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/windowed_with_patch.cmd" "wowmapview -w -p")
file(WRITE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/windowed_no_patch.cmd" "wowmapview -w -np")