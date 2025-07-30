project(sys_dumper)

file(GLOB SRC *.c *.cpp)
file(GLOB INC *.h *.hpp)

add_library(${PROJECT_NAME} SHARED ${SRC} ${INC})
target_link_libraries(${PROJECT_NAME} psapi)

# Deploy to game dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} ARGS -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "$ENV{COD4XCLIENT_CMAKE_GAME_ROOT}/ac_dumper.dll"
)
