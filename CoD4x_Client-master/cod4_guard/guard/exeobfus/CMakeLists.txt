project(exeobfus)

file(GLOB SRC *.c *.cpp)
file(GLOB INC *.h *.hpp)

add_executable(${PROJECT_NAME} ${SRC} ${INC})
target_link_libraries(${PROJECT_NAME} udis86)
