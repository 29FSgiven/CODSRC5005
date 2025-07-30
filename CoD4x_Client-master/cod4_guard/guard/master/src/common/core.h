#include "tiny_msg.h"

typedef void menuDef_t;
typedef int conChannel_t;

typedef struct
{
    void (*Com_Error)(int code, const char* fmt, ...);
	void (*Menus_OpenByName)(const char* menuStr);
	void (*Menus_CloseByName)(const char *menuStr);
	void (*Menu_Open)(menuDef_t *menu);
	void (*Menu_Close)(menuDef_t *menu);
	void (*UI_CloseAllMenus)();
	void (*Com_Printf)(conChannel_t channel, const char* fmt, ...);
	void (*Cbuf_AddText)(const char* text);
	void (*Cmd_AddCommand)(const char* cmdname, void *);
	void (*NetSendData)(msg_t*);
	int (*VerifyAnticheatSignature)(unsigned char* data, int filesize, const char* pem);
	void (**CL_Netchan_Encode)( byte *data, int cursize );
	void (**CL_Netchan_Decode)(byte *data, int length);
	void (*Deinitialize)(int mode);
	void (*DB_PureInfoSetCallbacks)(void (*DB_PureInfoDestroy)(), void (*DB_PureInfoBegin)(), void (*DB_PureInfoReadChunk)(long unsigned int));
    void (*SetErrorMessage)(const char*);
    void (*SetImpure)();

}ac_imports_t;

extern ac_imports_t imp;
