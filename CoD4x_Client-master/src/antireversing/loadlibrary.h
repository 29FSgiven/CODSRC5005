#include <Windows.h>

void *get_proc_address(HMODULE module, const char *proc_name);
void mem_free_library(HMODULE *module);
HMODULE mem_load_library(void *module_map);
