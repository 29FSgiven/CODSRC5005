//================================================================================
#include <string.h>
#include "tiny_msg.h"

#define assert(x)

void MSG_Init( msg_t *buf, byte *data, int length ) {

	buf->data = data;
	buf->maxsize = length;
	buf->overflowed = false;
	buf->cursize = 0;
	buf->readonly = false;
	buf->splitData = NULL;
	buf->splitSize = 0;
	buf->readcount = 0;
	buf->bit = 0;
	buf->lastRefEntity = 0;
}

//
// writing functions
//


void MSG_WriteByte( msg_t *msg, int c ) {
#ifdef PARANOID
	if (c < -128 || c > 127)
		Com_Error (ERR_FATAL, "MSG_WriteByte: range error");
#endif
	int8_t* dst;

	if ( msg->maxsize - msg->cursize < 1 ) {
		msg->overflowed = true;
		return;
	}
	dst = (int8_t*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(int8_t);
}


void MSG_WriteShort( msg_t *msg, int c ) {
#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Com_Error (ERR_FATAL, "MSG_WriteShort: range error");
#endif
	signed short* dst;

	if ( msg->maxsize - msg->cursize < 2 ) {
		msg->overflowed = true;
		return;
	}
	dst = (int16_t*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(short);
}

void MSG_WriteLong( msg_t *msg, int c ) {
	int32_t *dst;

	if ( msg->maxsize - msg->cursize < 4 ) {
		msg->overflowed = true;
		return;
	}
	dst = (int32_t*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(int32_t);
}

void MSG_WriteInt64(msg_t *msg, int64_t c)
{
	int64_t *dst;

	if ( msg->maxsize - msg->cursize < (int)sizeof(int64_t) ) {
		msg->overflowed = true;
		return;
	}
	dst = (int64_t*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(int64_t);
}

void MSG_WriteFloat( msg_t *msg, float c ) {
	float *dst;

	if ( msg->maxsize - msg->cursize < 4 ) {
		msg->overflowed = true;
		return;
	}
	dst = (float*)&msg->data[msg->cursize];
	*dst = c;
	msg->cursize += sizeof(float);
}

void MSG_WriteData( msg_t *buf, const void *data, int length ) {
	int i;
	for(i=0; i < length; i++){
		MSG_WriteByte(buf, ((byte*)data)[i]);
	}
}

void MSG_WriteString( msg_t *sb, const char *s ) {
	if ( !s ) {
		MSG_WriteData( sb, "", 1 );
	} else {
		unsigned int l = strlen( s );
		MSG_WriteData( sb, s, l + 1 );
	}
}


//============================================================

//
// reading functions
//

// returns -1 if no more characters are available

int MSG_GetByte(msg_t *msg, int where)
{
  if ( where + (int)sizeof(int8_t) <= msg->cursize )
  {
    return msg->data[where];
  }
  assert(msg->splitData);
  return msg->splitData[where - msg->cursize];
}

int MSG_GetShort(msg_t *msg, int where)
{
  if ( where + (int)sizeof(int16_t) <= msg->cursize )
  {
    return *(int16_t*)&msg->data[where];
  }

  assert(msg->splitData);

  if(where >= msg->cursize)
  {
    return *(int16_t*)&msg->splitData[where - msg->cursize];
  }

  int16_t c;
  int i;

  //2 bytes splitted up
  for(i = 0; i < 2; ++i)
  {
    ((byte*)&c)[i] = MSG_GetByte(msg, where +i);
  }
  return c;
}

int MSG_GetLong(msg_t *msg, int where)
{
  if ( where + (int)sizeof(int32_t) <= msg->cursize )
  {
    return *(int32_t*)&msg->data[where];
  }

  assert(msg->splitData);

  if(where >= msg->cursize)
  {
    return *(int32_t*)&msg->splitData[where - msg->cursize];
  }

  int32_t c;
  int i;

  //4 bytes splitted up
  for(i = 0; i < 4; ++i)
  {
    ((byte*)&c)[i] = MSG_GetByte(msg, where +i);
  }
  return c;
}

int64_t MSG_GetInt64(msg_t *msg, int where)
{
  if ( where + (int)sizeof(int64_t) <= msg->cursize )
  {
    return *(int64_t*)&msg->data[where];
  }

  assert(msg->splitData);

  if(where >= msg->cursize)
  {
    return *(int64_t*)&msg->splitData[where - msg->cursize];
  }

  int64_t c;
  int i;
  //8 bytes splitted up
  for(i = 0; i < 8; ++i)
  {
    ((byte*)&c)[i] = MSG_GetByte(msg, where +i);
  }
  return c;
}


int MSG_ReadByte( msg_t *msg ) {
	byte	c;

	if ( msg->readcount+ (int)sizeof(byte) > msg->splitSize + msg->cursize ) {
		msg->overflowed = true;
		return -1;
	}
	c = MSG_GetByte(msg, msg->readcount);

	msg->readcount += sizeof(byte);
	return c;
}

int MSG_ReadShort( msg_t *msg ) {
	int16_t	c;

	if ( msg->readcount + (int)sizeof(short) > msg->splitSize + msg->cursize ) {
		msg->overflowed = true;
		return -1;
	}	
	c = MSG_GetShort(msg, msg->readcount);

	msg->readcount += sizeof(short);
	return c;
}

int32_t MSG_ReadLong( msg_t *msg ) {
	int32_t	c;

	if ( msg->readcount + (int)sizeof(int32_t) > msg->cursize + msg->splitSize) {
		msg->overflowed = true;
		return -1;
	}	
	c = MSG_GetLong(msg, msg->readcount);

	msg->readcount += sizeof(int32_t);
	return c;

}

int64_t MSG_ReadInt64( msg_t *msg ) {
	int64_t		c;

	if ( msg->readcount+(int)sizeof(int64_t) > msg->cursize + msg->splitSize ) {
		msg->overflowed = true;
		return -1;
	}	
	c = MSG_GetInt64(msg, msg->readcount);

	msg->readcount += sizeof(int64_t);
	return c;

}

/*
int MSG_SkipToString( msg_t *msg, const char* string ) {
	byte c;

	do{
		c = MSG_ReadByte( msg );      // use ReadByte so -1 is out of bounds
		if ( c == -1 )
		{
			return false;
		}
		if(c == string[0] && !Q_strncmp(msg->data + msg->readcount, string, msg->cursize - msg->readcount))
		{
			return true;
		}
	}
	return false;
}
*/


char *MSG_ReadString( msg_t *msg, char* bigstring, int len ) {
	int l,c;

	l = 0;
	do {
		c = MSG_ReadByte( msg );      // use ReadByte so -1 is out of bounds
		if ( c == -1 || c == 0 ) {
			break;
		}
		bigstring[l] = c;
		l++;
	} while ( l < len - 1 );

	bigstring[l] = 0;

	return bigstring;
}

char *MSG_ReadStringLine( msg_t *msg, char* bigstring, int len ) {
	int		l,c;

	l = 0;
	do {
		c = MSG_ReadByte(msg);		// use ReadByte so -1 is out of bounds
		if (c == -1 || c == 0 || c == '\n') {
			break;
		}
		// translate all fmt spec to avoid crash bugs
		if ( c == '%' ) {
			c = '.';
		}
		bigstring[l] = c;
		l++;
	} while (l < len -1);
	
	bigstring[l] = 0;
	
	return bigstring;
}

void MSG_ReadData( msg_t *msg, void *data, int len ) {
	int		i;

	for (i=0 ; i<len ; i++) {
		((byte *)data)[i] = MSG_ReadByte (msg);
	}
}
