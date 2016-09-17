include_directories(include)
include_directories(${PROJECT_SOURCE_DIR}/${ENGINE_LIB}/include)

file(GLOB SOURCE_FILES src/*.cpp)
file(GLOB HEADER_FILES include/*.h)

add_executable(${TEST_APP} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${TEST_APP} ${ENGINE_LIB})

add_custom_command(TARGET ${TEST_APP} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:${TEST_APP}>)
