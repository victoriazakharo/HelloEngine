include_directories(src/${PLATFORM})
include_directories(include)

file(GLOB SOURCE_FILES src/*.cpp)
file(GLOB PLATFORM_SOURCE_FILES src/${PLATFORM}/*.cpp)
file(GLOB PLATFORM_HEADER_FILES src/${PLATFORM}/*.h)
file(GLOB EXPORT_HEADER_FILES include/*.h)

if(${PLATFORM} STREQUAL win32)
    set(PLATFORM_LIBRARY "")  
elseif(${PLATFORM} STREQUAL xcb)
    set(PLATFORM_LIBRARY dl xcb)  
endif()

add_library(${ENGINE_LIB} SHARED
    ${PLATFORM_SOURCE_FILES} ${SOURCE_FILES} 
    ${PLATFORM_HEADER_FILES} ${HEADER_FILES}
    ${EXPORT_HEADER_FILES} 
)

target_link_libraries(${ENGINE_LIB} ${PLATFORM_LIBRARY})
set_target_properties(${ENGINE_LIB} PROPERTIES LINKER_LANGUAGE CXX)
