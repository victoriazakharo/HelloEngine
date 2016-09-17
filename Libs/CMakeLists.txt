include_directories(zlib)
add_subdirectory(zlib)

set(ZLIB_ROOT ${CMAKE_SOURCE_DIR}/Libs/zlib)
set(ZLIB_INCLUDE_DIR ${CMAKE_BINARY_DIR}/Libs/zlib ${CMAKE_SOURCE_DIR}/Libs/zlib)
set(ZLIB_LIBRARY ${CMAKE_BINARY_DIR}/bin/zlib)
unset(AMD64 CACHE)
unset(ASM686 CACHE)

set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(ld-version-script OFF CACHE BOOL "" FORCE)

add_subdirectory(libpng)
unset(AWK CACHE)
unset(CMAKE_BACKWARDS_COMPATIBILITY CACHE)
unset(DFA_XTRA CACHE)
unset(EXECUTABLE_OUTPUT_PATH CACHE)
unset(PNG_SHARED CACHE)
unset(PNG_TEST CACHE)
unset(PNG_TESTS CACHE)
unset(PNG_STATIC CACHE)
unset(ld-version-script CACHE)
unset(PNGARG CACHE)
unset(PNG_DEBUG CACHE)
unset(PNG_FRAMEWORK CACHE)
unset(INSTALL_BIN_DIR CACHE)
unset(INSTALL_LIB_DIR CACHE)
unset(INSTALL_INC_DIR CACHE)
unset(INSTALL_MAN_DIR CACHE)
unset(INSTALL_PKGCONFIG_DIR CACHE)
unset(LIBRARY_OUTPUT_PATH CACHE)
unset(PNG_PREFIX CACHE)

 set_target_properties(png_static zlibstatic PROPERTIES POSITION_INDEPENDENT_CODE ON)

set_target_properties(genfiles example zlib minigzip PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

set_target_properties (zlibstatic png_static genfiles example zlib minigzip
    PROPERTIES
    FOLDER Libs
)
