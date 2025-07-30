project(seh C CXX)


file(GLOB_RECURSE SRC common/*.c common/*.cpp os/*.c)
file(GLOB_RECURSE INC common/*.h common/*.hpp os/*.h)
file(GLOB_RECURSE ASM_SRC os/*.s)

set_source_files_properties(${ASM_SRC} PROPERTIES COMPILE_FLAGS "-xassembler-with-cpp")

add_library(${PROJECT_NAME} ${SRC} ${INC} ${ASM_SRC})
