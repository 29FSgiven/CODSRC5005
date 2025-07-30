project(client_loader_stub CXX C)

file(GLOB ASM src/*.asm)
file(GLOB SRC src/*.c src/*.cpp)
file(GLOB INC src/*.h src/*.hpp)
file(GLOB ZLIB_SRC src/zlib/*.c src/zlib/*.cpp)
file(GLOB ZLIB_INC src/zlib/*.h src/zlib/*.hpp)

# Because cmake choses gcc as compiler instead of nasm.
set_source_files_properties(${ASM} PROPERTIES LANGUAGE ASM_NASM)

add_library(${PROJECT_NAME} SHARED ${SRC} ${INC} ${ZLIB_SRC} ${ZLIB_INC} ${ASM})
target_link_libraries(${PROJECT_NAME} tomcrypt tommath)
target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc)

# Deploy to game dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} ARGS -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "$ENV{COD4XCLIENT_CMAKE_GAME_ROOT}/mss32.dll"
)
