#ifndef __IIMPORTSTRUCT_H__
#define __IIMPORTSTRUCT_H__

typedef struct
{
	const char* name;
	int index;
}safeimportaccess_t;


typedef uint32_t** classptr_t;

#define CPPCALL __attribute__((thiscall))

#endif