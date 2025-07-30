//
// client to server
//
enum clc_ops_e {
	clc_move,				// [[usercmd_t]
	clc_moveNoDelta,		// [[usercmd_t]
	clc_clientCommand,		// [string] message
	clc_EOF,
	clc_nop,
	clc_download,
	clc_empty1,
	clc_empty2,
	clc_steamcommands,
	clc_statscommands,
	clc_empty3,
	clc_empty4,
    clc_acdata
};

//used by anticheat server and client
typedef enum
{
        ac_downloadfile,
        ac_downloadblock,
        ac_masterreadysignal,
		ac_internalcommand
}accmds_t;