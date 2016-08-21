cmake_minimum_required(VERSION 2.8)
set(NAME HelloEngine)
set(TEST_APP Test)
set(ENGINE_LIB Engine)
project(${NAME})

if(WIN32)
    set(PLATFORM win32)
    add_definitions(-DUSE_PLATFORM_WIN32_KHR)
else()
    set(PLATFORM xcb)   
    add_definitions(-DUSE_PLATFORM_XCB_KHR)
endif()

set(RENDER_API vulkan)
add_definitions(-DUSE_RENDER_VULKAN)

add_definitions(-std=c++0x)
add_definitions(-std=c++14)
add_definitions(-std=gnu++14)

add_subdirectory(${ENGINE_LIB})
add_subdirectory(${TEST_APP})           
