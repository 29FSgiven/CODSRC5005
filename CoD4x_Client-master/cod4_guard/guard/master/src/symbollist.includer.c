#include "checksumengine.h"

//here are the include files which define every minor version of cod4x updates
#include "..\symbollists\symbollist_18.5-20-228-19-55.h"

//Here getting all the minor versions added to array so they can get selected for each version properly
struct XAC_GameBinData *symbolversions[] = 
{
	&xac_gamebindata_18_5,
	NULL
};
