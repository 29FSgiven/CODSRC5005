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



#ifndef __SYS_NET_H__
#define __SYS_NET_H__

#include "q_shared.h"


/*
==============================================================

NET

==============================================================
*/

#define NET_ENABLEV4            0x01
#define NET_ENABLEV6            0x02
// if this flag is set, always attempt ipv6 connections instead of ipv4 if a v6 address is found.
#define NET_PRIOV6              0x04
// disables ipv6 multicast support if set.
#define NET_DISABLEMCAST        0x08
#define	PORT_ANY		-1


#ifndef _WIN32
	//#define SOCKET_DEBUG
#endif

typedef enum {
	NA_BAD = 0,					// an address lookup failed
	NA_BOT = 0,
	NA_LOOPBACK = 2,
	NA_BROADCAST = 3,
	NA_IP = 4,
	NA_IP6 = 5,
	NA_TCP = 6,
	NA_TCP6 = 7,
	NA_MULTICAST6 = 8,
	NA_UNSPEC = 9,
	NA_DOWN = 10,
} netadrtype_t;

typedef enum {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

#define NET_ADDRSTRMAXLEN 48	// maximum length of an IPv6 address string including trailing '\0'

typedef struct {
	netadrtype_t	type;
	int				scope_id;
	unsigned short port;
	unsigned short pad;
	int				sock;	//Socket FD. 0 = any socket
    union{
	    byte	ip[4];
	    byte	ip6[16];
	};
	int unsentbytes;
	unsigned char unsentbuffer[1024];
	void* security_ctx;
}netadr_t;

void		NET_Init( qboolean isserver, unsigned int port );
void		NET_Shutdown( void );
void		NET_Restart_f( void );
void		NET_Config( qboolean enableNetworking, qboolean isserver, unsigned int port );
void		NET_FlushPacketQueue(void);
qboolean	NET_SendPacket (netsrc_t sock, int length, const void *data, netadr_t *to);
void		QDECL NET_OutOfBandPrint( netsrc_t net_socket, netadr_t *adr, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
void		QDECL NET_OutOfBandData( netsrc_t sock, netadr_t *adr, byte *format, int len );
void		NET_RegisterDefaultCommunicationSocket(netadr_t *adr);
netadr_t*	NET_GetDefaultCommunicationSocket();
qboolean	NET_CompareAdr (netadr_t *a, netadr_t *b);
qboolean	NET_CompareBaseAdrMask(netadr_t *a, netadr_t *b, int netmask);
qboolean	NET_CompareBaseAdr (netadr_t *a, netadr_t *b);
qboolean	NET_IsLocalAddress (netadr_t adr);
const char	*NET_AdrToString (netadr_t *a);
const char	*NET_AdrToStringMT (netadr_t *a, char*, int);
const char	*NET_AdrToStringShort (netadr_t *a);
const char	*NET_AdrToStringwPort (netadr_t *a);
netadr_t	*NET_SockToAdr(int socket);
const char	*NET_AdrToConnectionString(netadr_t *a);
const char	*NET_AdrToConnectionStringShort(netadr_t *a);
const char	*NET_AdrToConnectionStringMask(netadr_t *a);
int		NET_StringToAdr ( const char *s, netadr_t *a, netadrtype_t family);
//qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, msg_t *net_message);
void		NET_JoinMulticast6(void);
void		NET_LeaveMulticast6(void);
__optimize3 __regparm1 void	NET_Sleep(unsigned int usec);
void NET_Clear(void);
const char*	NET_AdrMaskToString(netadr_t *adr);

qboolean	Sys_SendPacket( int length, const void *data, netadr_t *to );
qboolean	Sys_StringToAdr( const char *s, netadr_t *a, netadrtype_t family );

//Does NOT parse port numbers, only base addresses.
qboolean	Sys_IsLANAddress (netadr_t *adr);
void		Sys_ShowIP(void);

int _NET_TcpSendData(int sock, unsigned char* buf, int trysendlen, char* errormsg, int errormsglen);
void NET_TcpServerPacketEventLoop();
void NET_TcpServerRebuildFDList(void);
void NET_TcpServerInit(const char* privatekeyfile, const char* privatekeypassword, const char* certfiles);
int NET_TcpClientConnect( const char *remoteAdr );
int NET_TcpClientConnectToAdr( netadr_t* adr );
int NET_TcpClientConnectFromAdrToAdr( netadr_t* destination, netadr_t* source );
int NET_TcpClientConnectFromAdrToAdrSilent( netadr_t* destination, netadr_t* source );
int NET_TcpClientConnectNonBlockingToAdr( netadr_t* adr );
int NET_TcpClientGetData(int sock, unsigned char* buf, int maxlen, char* errormsg, int errormsglen);
void NET_TcpCloseSocket(int socket);
int NET_TcpIsSocketReady(int socket); //return: 1 ready, 0 not ready, -1 select error, -2 other error
const char* NET_GetHostAddress(char* adrstrbuf, int len);
int NET_GetHostPort();
netadr_t* NET_GetLocalAddressList(int* count);
qboolean Sys_IsReservedAddress( netadr_t *adr );

typedef enum {
	TCP_CONNWAIT,
	TCP_CONNSUCCESSFULL
}tcpclientstate_t;

void NET_TCPConnectionClosed(netadr_t* adr, uint64_t connectionId, uint64_t serviceId);
qboolean NET_TCPPacketEvent(netadr_t* remote, byte* bufData, int cursize, uint64_t *connectionId, uint64_t *serviceId);

#endif
