/*
===========================================================================
    Copyright (C) 2010-2013  Ninja and TheKelm
    Copyright (C) 1999-2005 Id Software, Inc.

    This file is part of CoD4X18-Server source code.

    CoD4X18-Server source code is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    CoD4X18-Server source code is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>
===========================================================================
*/



#include <string.h>

#include "cmd.h"

#define MAX_TOKENIZE_STRINGS 2

typedef struct{
	int currentString; //Count of parsed strings

	int cmd_argc; //Number of all parsed tokens in all strings combined
	int cmd_argcList[MAX_TOKENIZE_STRINGS]; //Number of parsed tokens in each string

	char *cmd_argv[MAX_STRING_TOKENS]; // points into cmd_tokenized. cmd_argc must point into the free space of cmd_tokenized
	char cmd_tokenized[BIG_INFO_STRING + MAX_STRING_TOKENS]; // will have 0 bytes inserted
}cmdTokenizer_t;

static cmdTokenizer_t tokenStrings;



/*
============
Cmd_Argc	Returns count of commandline arguments
============
*/
int	Cmd_Argc( void ) {

	if(tokenStrings.currentString == 0)
		return 0;

	return tokenStrings.cmd_argcList[tokenStrings.currentString -1];
}

/*
============
Cmd_Argv	Returns commandline argument by number
============
*/

char	*Cmd_Argv( int arg ) {

	int cmd_argc;
	int final_argc;

	cmd_argc = Cmd_Argc();

	if(cmd_argc == 0)
	    return "";

	if(cmd_argc - arg <= 0)
            return "";

        final_argc = tokenStrings.cmd_argc - cmd_argc + arg;

	if(tokenStrings.cmd_argv[final_argc] == NULL)
	    return "";

	return tokenStrings.cmd_argv[final_argc];
}


/*
============
Cmd_ArgvBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	Cmd_ArgvBuffer( int arg, char *buffer, int bufferLength ) {
	Q_strncpyz( buffer, Cmd_Argv(arg), bufferLength );
}



/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/

char	*Cmd_Args( char* buff, int bufsize ) {

	int		i;
	int		cmd_argc = Cmd_Argc();

	buff[0] = 0;
	for ( i = 1 ; i < cmd_argc ; i++ ) {
		Q_strcat( buff, bufsize, Cmd_Argv(i) );
		if ( i != cmd_argc-1 ) {
			Q_strcat( buff, bufsize, " " );
		}
	}

	return buff;
}


/*
============
Cmd_Argvs

Returns a single string containing argv(int arg) to argv(argc()-arg)
============
*/

char	*Cmd_Argsv( int arg, char* buff, int bufsize ) {

	int		i;
	int		cmd_argc = Cmd_Argc();
	buff[0] = 0;
	for ( i = arg ; i < cmd_argc ; i++ ) {
		Q_strcat( buff, bufsize, Cmd_Argv(i) );
		if ( i != cmd_argc-1 ) {
			Q_strcat( buff, bufsize, " " );
		}
	}

	return buff;
}

typedef struct{
	int cmd_argc;
	int availableBuf;
	char* cmd_tokenized;
	char** cmd_argv;
}cmdTokenizeParams_t;



void Cmd_EndTokenizedString( )
{
    if(tokenStrings.currentString <= 0)
    {
        preInitError("Cmd_EndTokenizedString( ): Attempt to free more tokenized strings than tokenized");
        return;
    }
    --tokenStrings.currentString;

    if(tokenStrings.currentString < MAX_TOKENIZE_STRINGS)
    {
        tokenStrings.cmd_argc -= tokenStrings.cmd_argcList[tokenStrings.currentString];
    }
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the apropriate place, The argv array
will point into this temporary buffer.
============
*/
// NOTE TTimo define that to track tokenization issues
//#define TKN_DBG
static void Cmd_TokenizeStringInternal( const char *text_in, qboolean ignoreQuotes, cmdTokenizeParams_t* param) {
	const char	*text;
	char		*textOut;

#ifdef TKN_DBG
  // FIXME TTimo blunt hook to try to find the tokenization of userinfo
  Com_DPrintf("Cmd_TokenizeString: %s\n", text_in);
#endif

	if ( !text_in ) {
		return;
	}

	text = text_in;
	textOut = param->cmd_tokenized;

	while ( 1 ) {
		if ( param->cmd_argc == MAX_STRING_TOKENS ) {
			Com_PrintError( "Cmd_TokenizeString(): MAX_STRING_TOKENS exceeded\n");
			return;			// this is usually something malicious
		}

		while ( 1 ) {
			// skip whitespace
			while ( *text != '\0' && (byte)*text <= ' ' ) {
				text++;
			}
			if ( *text == '\0' ) {
				return;			// all tokens parsed
			}

			// skip // comments
			if ( text[0] == '/' && text[1] == '/' ) {
				return;			// all tokens parsed
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				while ( *text && ( text[0] != '*' || text[1] != '/' ) ) {
					text++;
				}
				if ( *text == '\0') {
					return;		// all tokens parsed
				}
				text += 2;
			} else {
				break;			// we are ready to parse a token
			}
		}

		// handle quoted strings
    // NOTE TTimo this doesn't handle \" escaping
		if ( !ignoreQuotes && *text == '"' ) {
			param->cmd_argv[param->cmd_argc] = textOut;
			text++;
			while ( *text != '\0' && *text != '"' && param->availableBuf > 1) {
				*textOut++ = *text++;
				--param->availableBuf;
			}
			*textOut++ = 0;
			--param->availableBuf;
			if(param->availableBuf < 2)
			{
				Com_PrintError( "Cmd_TokenizeString(): length of tokenize buffer exceeded\n");
				return;
			}
			param->cmd_argc++;
			param->cmd_argv[param->cmd_argc] = textOut;
			if ( *text == '\0') {
				return;		// all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		param->cmd_argv[param->cmd_argc] = textOut;


		// skip until whitespace, quote, or command
		while ( (byte)*text > ' ' && param->availableBuf > 1) {
			if ( !ignoreQuotes && text[0] == '"' ) {
				break;
			}

			if ( text[0] == '/' && text[1] == '/' ) {
				break;
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				break;
			}

			*textOut++ = *text++;
			--param->availableBuf;
		}

		*textOut++ = 0;
		--param->availableBuf;

		if(param->availableBuf < 2)
		{
			Com_PrintError("Cmd_TokenizeString(): length of tokenize buffer exceeded\n");
			return;
		}

		param->cmd_argc++;
		param->cmd_argv[param->cmd_argc] = textOut;

		if ( *text == '\0') {
			return;		// all tokens parsed
		}
	}

}


/*
============
Cmd_TokenizeString
============
*/
static void Cmd_TokenizeString2( const char *text_in, qboolean ignore_quotes ) {

	cmdTokenizeParams_t param;
	int oldargc;
	int occupiedBuf;

	oldargc = tokenStrings.cmd_argc;

	if(tokenStrings.currentString < MAX_TOKENIZE_STRINGS)
	{

		param.cmd_argc = tokenStrings.cmd_argc;
		param.cmd_argv = tokenStrings.cmd_argv;

		if(tokenStrings.currentString > 0)
		{
			if(tokenStrings.cmd_argv[tokenStrings.cmd_argc] == NULL)
			{
				preInitError("Cmd_TokenizeString( ): Free string is a NULL pointer...");
			}
			param.cmd_tokenized = tokenStrings.cmd_argv[tokenStrings.cmd_argc];
		}else{
			param.cmd_tokenized = tokenStrings.cmd_tokenized;
		}

		occupiedBuf = param.cmd_tokenized - tokenStrings.cmd_tokenized;
		param.availableBuf = sizeof(tokenStrings.cmd_tokenized) - occupiedBuf;

		Cmd_TokenizeStringInternal( text_in, ignore_quotes, &param );

		tokenStrings.cmd_argcList[tokenStrings.currentString] = param.cmd_argc - oldargc;
		tokenStrings.cmd_argc = param.cmd_argc;
	}else{
		Com_PrintError("Cmd_TokenizeString(): MAX_TOKENIZE_STRINGS exceeded\n");
	}
	tokenStrings.currentString++;
}

/*
============
Cmd_TokenizeStringIgnoreQuotes
============
*/
void Cmd_TokenizeStringIgnoreQuotes( const char *text_in ) {
	Cmd_TokenizeString2( text_in, qtrue );
}


/*
============
Cmd_TokenizeString
============
*/
void Cmd_TokenizeString( const char *text_in ) {
	Cmd_TokenizeString2( text_in, qfalse);
}
