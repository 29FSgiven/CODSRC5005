#include <stdint.h>
#include <stdbool.h>

typedef unsigned char byte;
typedef unsigned int qboolean;

typedef struct {
	qboolean overflowed;		//0x00
	qboolean	readonly;		//0x04
	byte		*data;			//0x08
	byte		*splitData;		//0x0c
	int		maxsize;		//0x10
	int		cursize;		//0x14
	int		splitSize;		//0x18
	int		readcount;		//0x1c
	int		bit;			//0x20	// for bitwise reads and writes
	int		lastRefEntity;		//0x24
} msg_t; //Size: 0x28


void MSG_Init( msg_t *buf, byte *data, int length );
void MSG_InitReadOnly( msg_t *buf, byte *data, int length );
void MSG_InitReadOnlySplit( msg_t *buf, byte *data, int length, byte*, int );
void MSG_Clear( msg_t *buf ) ;
void MSG_BeginReading( msg_t *msg ) ;
void MSG_Copy(msg_t *buf, byte *data, int length, msg_t *src);
void MSG_WriteByte( msg_t *msg, int c ) ;
void MSG_WriteShort( msg_t *msg, int c ) ;
void MSG_WriteLong( msg_t *msg, int c ) ;
void MSG_WriteData( msg_t *buf, const void *data, int length ) ;
void MSG_WriteString( msg_t *sb, const char *s ) ;
void MSG_WriteBigString( msg_t *sb, const char *s ) ;
int MSG_ReadByte( msg_t *msg ) ;
int MSG_ReadShort( msg_t *msg ) ;
int MSG_ReadLong( msg_t *msg ) ;
char *MSG_ReadString( msg_t *msg, char* bigstring, int len );
char *MSG_ReadStringLine( msg_t *msg, char* bigstring, int len );
void MSG_ReadData( msg_t *msg, void *data, int len ) ;

void MSG_Discard(msg_t* msg);

void MSG_WriteInt64(msg_t *msg, int64_t c);
int64_t MSG_ReadInt64( msg_t *msg );
