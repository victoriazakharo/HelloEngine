include_directories(src/${PLATFORM})
include_directories(src/${RENDER_API})
include_directories(${CMAKE_SOURCE_DIR}/Libs)
include_directories(${CMAKE_BINARY_DIR}/Libs/libpng)
include_directories(include)

file(GLOB SOURCE_FILES src/*.cpp)
file(GLOB PLATFORM_SOURCE_FILES src/${PLATFORM}/*.cpp)
file(GLOB PLATFORM_HEADER_FILES src/${PLATFORM}/*.h)
file(GLOB EXPORT_HEADER_FILES include/*.h)
file(GLOB RENDER_API_FILES src/${RENDER_API}/*)

if(${PLATFORM} STREQUAL win32)
    set(PLATFORM_LIBRARY "")  
	 if(${RENDER_API} STREQUAL vulkan)
        add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
    endif()
elseif(${PLATFORM} STREQUAL xcb)
	if(${RENDER_API} STREQUAL vulkan)
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
        add_definitions(-DVK_NO_PROTOTYPES)
    endif()
    set(PLATFORM_LIBRARY dl xcb)     
endif()

add_library(${ENGINE_LIB} SHARED
    ${PLATFORM_SOURCE_FILES} ${SOURCE_FILES} 
    ${PLATFORM_HEADER_FILES} ${HEADER_FILES}
	${RENDER_API_FILES}
    ${EXPORT_HEADER_FILES} 
)

include (GenerateExportHeader)
GENERATE_EXPORT_HEADER(${ENGINE_LIB}
             BASE_NAME ${NAME}
             EXPORT_MACRO_NAME HELLO_ENGINE_API
             EXPORT_FILE_NAME ${PROJECT_SOURCE_DIR}/${ENGINE_LIB}/include/hello_export.h)
  
target_link_libraries(${ENGINE_LIB} ${PLATFORM_LIBRARY} $<TARGET_FILE:png_static> $<TARGET_FILE:zlibstatic>)
add_dependencies(${ENGINE_LIB} png_static zlibstatic)

add_custom_command(TARGET ${ENGINE_LIB} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${ENGINE_LIB}>)
