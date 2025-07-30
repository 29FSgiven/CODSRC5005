project(jpg)

add_library(${PROJECT_NAME} STATIC compressor.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:FASTCOMPILE>)

###############################################################################
project(diskinfo)

add_library(${PROJECT_NAME} STATIC system_patch.c)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:FASTCOMPILE>)
