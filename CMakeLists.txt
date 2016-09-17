cmake_minimum_required(VERSION 2.8)
set(NAME HelloEngine)
set(TEST_APP Test)
set(ENGINE_LIB Engine)
set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
project(${NAME})

if(WIN32)
    set(PLATFORM win32)
    add_definitions(-DUSE_PLATFORM_WIN32_KHR)
	set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER CMake)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Debug;Release")
else()
    set(PLATFORM xcb)   
	add_definitions(-std=c++11)
    add_definitions(-DUSE_PLATFORM_XCB_KHR)
endif()

set(RENDER_API vulkan)
add_definitions(-DUSE_RENDER_VULKAN)
add_subdirectory(Libs)
add_subdirectory(${ENGINE_LIB})
add_subdirectory(${TEST_APP}) 

set_target_properties(${ENGINE_LIB} ${TEST_APP} zlibstatic png_static
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR}
)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set_target_properties(${ENGINE_LIB} ${TEST_APP} zlibstatic png_static
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BINARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BINARY_DIR}
    )
endforeach()
