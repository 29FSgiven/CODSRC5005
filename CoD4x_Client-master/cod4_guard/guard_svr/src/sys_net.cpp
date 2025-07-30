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

#include "sys_net.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <stddef.h>		/* for offsetof*/
#include <sys/time.h>


#ifdef _WIN32
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include <iphlpapi.h>
#	include <wincrypt.h>
#	if WINVER < 0x501
#		ifdef __MINGW32__
			// wspiapi.h isn't available on MinGW, so if it's
			// present it's because the end user has added it
			// and we should look for it in our tree
#			include "wspiapi.h"
#		else
#			include <wspiapi.h>
#		endif
#	else
#		include <ws2spi.h>
#	endif

typedef int socklen_t;
#	ifdef ADDRESS_FAMILY
#		define sa_family_t	ADDRESS_FAMILY
#	else
typedef unsigned short sa_family_t;
#	endif

/* no epipe yet */
#ifndef WSAEPIPE
    #define WSAEPIPE       -12345
#endif
#	define EAGAIN			WSAEWOULDBLOCK
#	define EADDRNOTAVAIL		WSAEADDRNOTAVAIL
#	define EAFNOSUPPORT		WSAEAFNOSUPPORT
#	define ECONNRESET		WSAECONNRESET
#	define EINPROGRESS		WSAEINPROGRESS
#	define EINTR			WSAEINTR
# define EPIPE      WSAEPIPE
typedef u_long	ioctlarg_t;
#	define socketError		WSAGetLastError( )

#define NET_NOSIGNAL 0x0

static WSADATA	winsockdata;
static qboolean	winsockInitialized = qfalse;

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY           27 // Treat wildcard bind as AF_INET6-only.
#endif 

int inet_pton(int af, const char *src, void *dst)
{
	struct sockaddr_storage sin;
	int addrSize = sizeof(sin);
	char address[256];
	strncpy(address, src, sizeof(address));

	int rc = WSAStringToAddressA( address, af, NULL, (SOCKADDR*)&sin, &addrSize ); 
	if(rc != 0)
	{
		return -1;
	}
	if(af == AF_INET)
	{
		*((struct in_addr *)dst) = ((struct sockaddr_in*)&sin)->sin_addr;
		return 1;
	}
	if(af == AF_INET6)
	{
		*((struct in_addr6 *)dst) = ((struct sockaddr_in6*)&sin)->sin6_addr;
		return 1;
	}
	return 0;
}

#else

#	if MAC_OS_X_VERSION_MIN_REQUIRED == 1020
		// needed for socklen_t on OSX 10.2
#		define _BSD_SOCKLEN_T_
#	endif

#ifdef MACOS_X
    #define NET_NOSIGNAL SO_NOSIGPIPE
#else
    #define NET_NOSIGNAL MSG_NOSIGNAL
#endif

#	include <sys/socket.h>
#	include <errno.h>
#	include <netdb.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <net/if.h>
#	include <sys/ioctl.h>
#	include <sys/types.h>
#	include <sys/time.h>
#	include <unistd.h>
#	if !defined(__sun) && !defined(__sgi)
#		include <ifaddrs.h>
#	endif

#	ifdef __sun
#		include <sys/filio.h>
#	endif


#	define INVALID_SOCKET		-1
#	define SOCKET_ERROR		-1
#	define closesocket		close
#	define ioctlsocket		ioctl
typedef int	ioctlarg_t;
#	define socketError		errno
typedef int SOCKET;
#endif


#ifndef MAX_MSGLEN
#define MAX_MSGLEN 0x20000
#endif

#ifndef __CMD_H__
void Cmd_AddCommand(char* name, void* fun){}
#endif

#ifdef Com_GetFrameMsec
#define NET_TimeGetTime Com_GetFrameMsec
#endif

#ifndef __NET_GAME_H__
unsigned int net_timeBase;
unsigned int NET_TimeGetTime( void )
{
	struct timeval tp;
	gettimeofday( &tp, NULL );
	if ( !net_timeBase )
		net_timeBase = tp.tv_sec;

	return ( tp.tv_sec - net_timeBase ) * 1000 + tp.tv_usec / 1000;
}
#endif

/*
#ifndef __NET_GAME_H__
#pragma message "Function NET_TCPConnectionClosed is undefined"
void NET_TCPConnectionClosed(netadr_t* adr, uint64_t connectionId, uint64_t serviceId){}
#endif
#ifndef __NET_GAME_H__
#pragma message "Function NET_TCPPacketEvent is undefined"
qboolean NET_TCPPacketEvent(netadr_t* remote, byte* bufData, int cursize, uint64_t *connectionId, uint64_t *serviceId){ return qfalse; }
#endif
*/

static int networkingEnabled = 0;

typedef struct{

    SOCKET sock;
    int localipindex;

}socketData_t;

static SOCKET	tcp_socket = INVALID_SOCKET;

static unsigned int net_port;

#ifndef IF_NAMESIZE
  #define IF_NAMESIZE 16
#endif


#define MAX_TCPCONNECTIONS MAX_CONNECTEDCLIENTS
#define MIN_TCPCONNWAITTIME 320
#define MAX_TCPCONNWAITTIME 3000
#define MAX_TCPCONNECTEDTIMEOUT 1800000 //30 minutes - close this if we have too many waiting connections

typedef struct{
	netadr_t		remote;
	unsigned int	lastMsgTime;
	uint64_t		connectionId;
	uint64_t		serviceId;
	tcpclientstate_t	state;
	qboolean		wantwrite;
	//SOCKET			sock;
}tcpConnections_t;


typedef struct{
	fd_set			fdr, fdw;
	int			highestfd;
	int			activeConnectionCount; //Connections that have been successfully authentificated
	tcpConnections_t	connections[MAX_TCPCONNECTIONS];
}tcpServer_t;

tcpServer_t tcpServer;

void NET_ShutdownDNS();
qboolean NET_ResolveBlocking(const char *domain, struct sockaddr_storage *outaddr, sa_family_t family);


/*
====================
NET_ErrorString
====================
*/
const char *NET_ErrorString( void ) {
#ifdef _WIN32
	//FIXME: replace with FormatMessage?
	switch( socketError ) {
		case WSAEINTR: return "WSAEINTR";
		case WSAEBADF: return "WSAEBADF";
		case WSAEACCES: return "WSAEACCES";
		case WSAEDISCON: return "WSAEDISCON";
		case WSAEFAULT: return "WSAEFAULT";
		case WSAEINVAL: return "WSAEINVAL";
		case WSAEMFILE: return "WSAEMFILE";
		case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
		case WSAEINPROGRESS: return "WSAEINPROGRESS";
		case WSAEALREADY: return "WSAEALREADY";
		case WSAENOTSOCK: return "WSAENOTSOCK";
		case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
		case WSAEMSGSIZE: return "WSAEMSGSIZE";
		case WSAEPROTOTYPE: return "WSAEPROTOTYPE";
		case WSAENOPROTOOPT: return "WSAENOPROTOOPT";
		case WSAEPROTONOSUPPORT: return "WSAEPROTONOSUPPORT";
		case WSAESOCKTNOSUPPORT: return "WSAESOCKTNOSUPPORT";
		case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
		case WSAEPFNOSUPPORT: return "WSAEPFNOSUPPORT";
		case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
		case WSAEADDRINUSE: return "WSAEADDRINUSE";
		case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
		case WSAENETDOWN: return "WSAENETDOWN";
		case WSAENETUNREACH: return "WSAENETUNREACH";
		case WSAENETRESET: return "WSAENETRESET";
		case WSAECONNABORTED: return "WSWSAECONNABORTEDAEINTR";
		case WSAECONNRESET: return "WSAECONNRESET";
		case WSAENOBUFS: return "WSAENOBUFS";
		case WSAEISCONN: return "WSAEISCONN";
		case WSAENOTCONN: return "WSAENOTCONN";
		case WSAESHUTDOWN: return "WSAESHUTDOWN";
		case WSAETOOMANYREFS: return "WSAETOOMANYREFS";
		case WSAETIMEDOUT: return "WSAETIMEDOUT";
		case WSAECONNREFUSED: return "WSAECONNREFUSED";
		case WSAELOOP: return "WSAELOOP";
		case WSAENAMETOOLONG: return "WSAENAMETOOLONG";
		case WSAEHOSTDOWN: return "WSAEHOSTDOWN";
		case WSASYSNOTREADY: return "WSASYSNOTREADY";
		case WSAVERNOTSUPPORTED: return "WSAVERNOTSUPPORTED";
		case WSANOTINITIALISED: return "WSANOTINITIALISED";
		case WSAHOST_NOT_FOUND: return "WSAHOST_NOT_FOUND";
		case WSATRY_AGAIN: return "WSATRY_AGAIN";
		case WSANO_RECOVERY: return "WSANO_RECOVERY";
		case WSANO_DATA: return "WSANO_DATA";
		default: return "NO ERROR";
	}
#else
	return strerror(socketError);
#endif
}

const char* NET_ErrorStringMT(char* errstr, int maxlen)
{
    Q_strncpyz(errstr, NET_ErrorString( ), maxlen);
    return errstr;
}


static void NetadrToSockadr( netadr_t *a, struct sockaddr *s ) {
	if( a->type == NA_BROADCAST ) {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_port = a->port;
		((struct sockaddr_in *)s)->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if( a->type == NA_IP || a->type == NA_TCP ) {
		((struct sockaddr_in *)s)->sin_family = AF_INET;
		((struct sockaddr_in *)s)->sin_addr.s_addr = *(int *)&a->ip;
		((struct sockaddr_in *)s)->sin_port = a->port;
	}
	else if( a->type == NA_IP6 || a->type == NA_TCP6 ) {
		((struct sockaddr_in6 *)s)->sin6_family = AF_INET6;
		((struct sockaddr_in6 *)s)->sin6_addr = * ((struct in6_addr *) &a->ip6);
		((struct sockaddr_in6 *)s)->sin6_port = a->port;
		((struct sockaddr_in6 *)s)->sin6_scope_id = a->scope_id;
	}

}


__optimize3 __regparm3 static void SockadrToNetadr( struct sockaddr *s, netadr_t *a, qboolean tcp, int socket) {
	if (s->sa_family == AF_INET) {
		if(!tcp)
			a->type = NA_IP;
		else
			a->type = NA_TCP;

		*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
		a->port = ((struct sockaddr_in *)s)->sin_port;
	}
	else if(s->sa_family == AF_INET6)
	{
		if(!tcp)
			a->type = NA_IP6;
		else
			a->type = NA_TCP6;

		memcpy(a->ip6, &((struct sockaddr_in6 *)s)->sin6_addr, sizeof(a->ip6));
		a->port = ((struct sockaddr_in6 *)s)->sin6_port;
		a->scope_id = ((struct sockaddr_in6 *)s)->sin6_scope_id;
	}
	a->sock = socket;
}


static struct addrinfo *SearchAddrInfo(struct addrinfo *hints, sa_family_t family)
{
	while(hints)
	{
		if(hints->ai_family == family)
			return hints;

		hints = hints->ai_next;
	}

	return NULL;
}


static qboolean Sys_StringToSockaddr(const char *s, struct sockaddr *sadr, int sadr_len, sa_family_t family)
{
	struct addrinfo hints;
	struct addrinfo *res = NULL;
	struct addrinfo *search = NULL;
	struct addrinfo *hintsp;
	int retval;

	memset(sadr, '\0', sizeof(*sadr));
	memset(&hints, '\0', sizeof(hints));

	hintsp = &hints;
	hintsp->ai_family = family;
	hintsp->ai_socktype = SOCK_DGRAM;

	retval = getaddrinfo(s, NULL, hintsp, &res);

	if(!retval)
	{
		if(family == AF_UNSPEC)
		{
			// Decide here and now which protocol family to use
			search = SearchAddrInfo(res, AF_INET6);

			if(!search){
				search = SearchAddrInfo(res, AF_INET);
			}
		}
		else
			search = SearchAddrInfo(res, family);

		if(search)
		{
			if(search->ai_addrlen > (unsigned int)sadr_len)
				search->ai_addrlen = (unsigned int)sadr_len;

			memcpy(sadr, search->ai_addr, search->ai_addrlen);
			freeaddrinfo(search);

			return qtrue;
		}
		else
			Com_PrintError("Sys_StringToSockaddr: Error resolving %s: No address of required type found.\n", s);
	}
	else
		Com_PrintError("Sys_StringToSockaddr: Error resolving %s: %s\n", s, gai_strerror(retval));

	if(res)
		freeaddrinfo(res);

	return qfalse;
}


/*
=============
Sys_SockaddrToString
=============
*/
static void Sys_SockaddrToString(char *dest, int destlen, struct sockaddr *input)
{
	socklen_t inputlen;

	if (input->sa_family == AF_INET6)
		inputlen = sizeof(struct sockaddr_in6);
	else
		inputlen = sizeof(struct sockaddr_in);

	if(getnameinfo(input, inputlen, dest, destlen, NULL, 0, NI_NUMERICHOST) && destlen > 0)
		*dest = '\0';
}

/*
=============
Sys_StringToAdr
=============
*/
qboolean Sys_StringToAdr( const char *s, netadr_t *a, netadrtype_t family ) {
	struct sockaddr_storage sadr;
	sa_family_t fam;
	qboolean tcp = qfalse;

	switch(family)
	{
		case NA_TCP:
			tcp = qtrue;
		case NA_IP:
			fam = AF_INET;

		break;

		case NA_TCP6:
			tcp = qtrue;
		case NA_IP6:
			fam = AF_INET6;

		break;

		default:
			fam = AF_UNSPEC;
		break;
	}

	if( !Sys_StringToSockaddr(s, (struct sockaddr *) &sadr, sizeof(sadr), fam ) ) {
		return qfalse;
	}

	SockadrToNetadr( (struct sockaddr *) &sadr, a, tcp, 0);

	return qtrue;
}


/*
===================
NET_CompareBaseAdrMask

Compare without port, and up to the bit number given in netmask.
===================
*/
qboolean NET_CompareBaseAdrMask(netadr_t *a, netadr_t *b, int netmask)
{
	byte cmpmask, *addra, *addrb;
	int curbyte;

	if (a->type != b->type)
		return qfalse;

	if (a->type == NA_LOOPBACK)
		return qtrue;

	if(a->type == NA_IP || a->type == NA_TCP)
	{
		addra = (byte *) &a->ip;
		addrb = (byte *) &b->ip;

		if(netmask < 0 || netmask > 32)
			netmask = 32;
	}
	else if(a->type == NA_IP6 || a->type == NA_TCP6)
	{
		addra = (byte *) &a->ip6;
		addrb = (byte *) &b->ip6;

		if(netmask < 0 || netmask > 128)
			netmask = 128;
	}
	else
	{
		Com_PrintError ("NET_CompareBaseAdr: bad address type\n");
		return qfalse;
	}

	curbyte = netmask >> 3;

	if(curbyte && memcmp(addra, addrb, curbyte))
			return qfalse;

	netmask &= 0x07;
	if(netmask)
	{
		cmpmask = (1 << netmask) - 1;
		cmpmask <<= 8 - netmask;

		if((addra[curbyte] & cmpmask) == (addrb[curbyte] & cmpmask))
			return qtrue;
	}
	else
		return qtrue;

	return qfalse;
}


/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean NET_CompareBaseAdr (netadr_t *a, netadr_t *b)
{
	return NET_CompareBaseAdrMask(a, b, -1);
}

const char	*NET_AdrToStringShortInternal (netadr_t *a, char* s, int len)
{
  if(len < NET_ADDRSTRMAXLEN)
  {
    Q_strncpyz(s, "buf < NET_ADDRSTRMAXLEN", len);
    return s;
  }

	if(a == NULL)
		return "(null)";

	if (a->type == NA_LOOPBACK)
		Com_sprintf (s, len, "loopback");
	else if (a->type == NA_BOT)
		Com_sprintf (s, len, "bot");
	else if (a->type == NA_IP || a->type == NA_IP6 || a->type == NA_TCP || a->type == NA_TCP6 )
	{
		struct sockaddr_storage sadr;

		memset(&sadr, 0, sizeof(sadr));
		NetadrToSockadr(a, (struct sockaddr *) &sadr);
		Sys_SockaddrToString(s, len, (struct sockaddr *) &sadr);
	}
	return s;
}

const char	*NET_AdrToStringShort(netadr_t *a)
{
  static	char	s[NET_ADDRSTRMAXLEN];
  return NET_AdrToStringShortInternal(a, s, sizeof(s));
}

__cdecl const char	*NET_AdrToStringShortMT(netadr_t *a, char* buf, int len)
{
  return NET_AdrToStringShortInternal(a, buf, len);
}

const char	*NET_AdrToStringInternal(netadr_t *a, char* s, int len)
{
	char		t[NET_ADDRSTRMAXLEN];
	struct 		sockaddr_storage sadr;

  if(len < NET_ADDRSTRMAXLEN)
  {
    Q_strncpyz(s, "buf < NET_ADDRSTRMAXLEN", len);
    return s;
  }

	if(a == NULL)
		return "(null)";

	if (a->type == NA_LOOPBACK){
		Com_sprintf (s, len, "loopback");
	}else if (a->type == NA_BOT){
		Com_sprintf (s, len, "bot");
	}else if(a->type == NA_IP || a->type == NA_TCP){

		memset(&sadr, 0, sizeof(sadr));
		NetadrToSockadr(a, (struct sockaddr *) &sadr);
		Sys_SockaddrToString(t, sizeof(t), (struct sockaddr *) &sadr);
		Com_sprintf(s, len, "%s:%hu", t, ntohs(a->port));

	}else if(a->type == NA_IP6 || a->type == NA_TCP6){

		memset(&sadr, 0, sizeof(sadr));
		NetadrToSockadr(a, (struct sockaddr *) &sadr);
		Sys_SockaddrToString(t, sizeof(t), (struct sockaddr *) &sadr);
		Com_sprintf(s, len, "[%s]:%hu", t, ntohs(a->port));
        }
	return s;
}

const char	*NET_AdrToString(netadr_t *a)
{
  static	char	s[NET_ADDRSTRMAXLEN];
  return NET_AdrToStringInternal(a, s, sizeof(s));
}

__cdecl const char	*NET_AdrToStringMT(netadr_t *a, char* buf, int len)
{
  return NET_AdrToStringInternal(a, buf, len);
}

qboolean	NET_CompareAdr (netadr_t *a, netadr_t *b)
{
	if(!NET_CompareBaseAdr(a, b))
		return qfalse;

	if (a->type == NA_IP || a->type == NA_IP6 || a->type == NA_TCP || a->type == NA_TCP6)
	{
		if (a->port == b->port)
			return qtrue;
	}
	else
		return qtrue;

	return qfalse;
}


//=============================================================================

/*
====================
NET_IPSocket
====================
*/

int NET_IPSocket( int port, int *err)
{
	SOCKET				newsocket;
	struct sockaddr_in6		address;
	ioctlarg_t			_true = 1;
	int				option;
	char				errstr[256];
//	struct	linger			so_linger;

	*err = 0;

	Com_Printf( "Opening IP socket at port: %hu\n", port );

	newsocket = socket( AF_INET6, SOCK_STREAM, IPPROTO_TCP );

	if( newsocket == INVALID_SOCKET ) {
		*err = socketError;
		Com_Printf( "WARNING: NET_IPSocket: socket: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
		return newsocket;
	}

	// make it non-blocking
	if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_IPSocket: ioctl FIONBIO: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
		*err = socketError;
		closesocket(newsocket);
		return INVALID_SOCKET;
	}

	option = 0;

	if( setsockopt( newsocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &option, sizeof(option) ) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_IPSocket: setsockopt IPV6_V6ONLY: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
	}

	option = 1;

	if( setsockopt( newsocket, SOL_SOCKET, SO_REUSEADDR, (char *) &option, sizeof(option) ) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_IPSocket: setsockopt SO_REUSEADDR: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
	}

	memset(&address, 0, sizeof(address));

	address.sin6_family = AF_INET6;

	if( port == PORT_ANY ) {
		address.sin6_port = 0;
	}
	else {
		address.sin6_port = htons( (short)port );
	}

	if( bind( newsocket, (const sockaddr *)&address, sizeof(address) ) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_IPSocket: bind: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}
	// Listen
	if( listen( newsocket, 96) == SOCKET_ERROR ) {
		Com_PrintWarning( "NET_IPSocket: listen: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
		*err = socketError;
		closesocket( newsocket );
		return INVALID_SOCKET;
	}
	return newsocket;
}


/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP( void ) {
	int		err;
	int		port;

	port = net_port;
	
	tcp_socket = NET_IPSocket( port, &err);

	if(tcp_socket == INVALID_SOCKET)
	{
		Com_PrintError("Could not bind to a IPv4 or IPv6 network socket");
	}
}


//===================================================================


/*
====================
NET_Config
====================
*/
void NET_Config( qboolean enableNetworking, qboolean isserver, unsigned int port) {
	qboolean	stop;
	qboolean	start;
	int		i;


	// if enable state is the same and no cvars were modified, we have nothing to do
	if( enableNetworking == networkingEnabled && net_port == port ) {
		return;
	}

	net_port = port;

	if( enableNetworking == networkingEnabled ) {
		if( enableNetworking ) {
			stop = qtrue;
			start = qtrue;
		}
		else {
			stop = qfalse;
			start = qfalse;
		}
	}
	else {
		if( enableNetworking ) {
			stop = qfalse;
			start = qtrue;
		}
		else {
			stop = qtrue;
			start = qfalse;
		}
		networkingEnabled = enableNetworking;
	}

	if( stop ) {

		tcpConnections_t *con;


		for(i = 0, con = tcpServer.connections; i < MAX_TCPCONNECTIONS; i++, con++){

			if(con->lastMsgTime > 0 && con->remote.sock != INVALID_SOCKET)
			{
				Com_Printf("Close TCP serversocket: %d\n", con->remote.sock);
				NET_TcpCloseSocket(con->remote.sock);
			}
		}

		if ( tcp_socket != INVALID_SOCKET ) {
			Com_Printf("Close TCP socket: %d\n", tcp_socket);
			closesocket( tcp_socket );
			tcp_socket = INVALID_SOCKET;
		}

#ifdef _WIN32
		WSACleanup( );
#endif
	}

	if( start )
	{

#ifdef _WIN32
		int		r;

		r = WSAStartup( MAKEWORD( 1, 1 ), &winsockdata );
		if( r ) {
			Com_PrintWarning( "Winsock initialization failed, returned %d\n", r );
			return;
		}

		winsockInitialized = qtrue;
		Com_Printf( "Winsock Initialized\n" );
#endif
		if(isserver)
		{
			NET_OpenIP();
		}else{
			tcp_socket = INVALID_SOCKET;
		}
	}
}

/*========================================================================================================
Functions for TCP networking which can be used by client and server
*/


/*
===============
NET_TcpCloseSocket

This function should be able to close all types of open TCP sockets
===============
*/

void NET_TcpCloseSocket(int socket)
{
        int i;
	tcpConnections_t	*conn;

	if(socket == INVALID_SOCKET)
		return;

	//Close the socket
	closesocket(socket);

	//See if this was a serversocket and clear all references to it
	for(i = 0, conn = tcpServer.connections; i < MAX_TCPCONNECTIONS; i++, conn++)
	{
		if(conn->remote.sock == socket)
		{
			conn->lastMsgTime = 0;
			FD_CLR(conn->remote.sock, &tcpServer.fdr);
			if(conn->wantwrite)
			{
				FD_CLR(conn->remote.sock, &tcpServer.fdw);
			}
			conn->state = TCP_CONNWAIT;

			tcpServer.activeConnectionCount--;
			NET_TCPConnectionClosed(&conn->remote, conn->connectionId, conn->serviceId);
			conn->remote.sock = INVALID_SOCKET;
			NET_TcpServerRebuildFDList();
			return;
		}
	}
}

/*
==================
NET_TcpSendData
Only for Stream sockets (TCP)
Return -1 if an fatal error happened on this socket otherwise 0
==================
*/

int _NET_TcpSendData( int sock, unsigned char *data, int length, char* errormsg, int maxerrorlen ) {

	int state, err;
	char errstr[256];
	if(sock < 1)
		return -1;

	state = send( sock, (const char*)data, length, NET_NOSIGNAL); // FIX: flag NOSIGNAL prevents SIGPIPE in case of connection problems

	if(state == SOCKET_ERROR)
	{
	  err = socketError;
	  if(err == EAGAIN || err == EINTR)
	  {
				return NET_WANT_WRITE;
	  }
      if(errormsg)
      {
        Q_strncpyz(errormsg, NET_ErrorStringMT(errstr, sizeof(errstr)), maxerrorlen);
      }
      if(err == EPIPE || err == ECONNRESET){
        return NET_CONNRESET;
	  }
	  return -1;
	}
	return state;
}

/*========================================================================================================
Functions for TCP networking which can be used only by server
*/

/*
==================
NET_TcpServerGetPAcket
Only for Stream sockets (TCP)
Returns the number of received bytes
==================
*/


int NET_TcpServerGetPacket(tcpConnections_t *conn, void *netmsg, int maxsize, qboolean warn){

	int err;
	int ret;
	char errstr[256];
	char adrstr[256];

	ret = recv(conn->remote.sock, (char*)netmsg, maxsize , 0);

	if(ret == SOCKET_ERROR){

		err = socketError;

		if(err == EAGAIN)
		{
			return 0; //Nothing more to read left
		}

		if(ret == ECONNRESET){

			if(warn){
				Com_Printf("Connection closed by: %s\n", NET_AdrToStringMT(&conn->remote, adrstr, sizeof(adrstr)));

				//Connection closed
			}
		}else
			Com_PrintWarning("NET_GetTcpPacket recv() syscall failed: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr))); // BUGFIX: this causes SIGSEGV in case of an error during console stream

		NET_TcpCloseSocket(conn->remote.sock);
		return -1;

	}else if(ret == 0){

		if(conn->state >= TCP_CONNSUCCESSFULL)
		{
			Com_Printf("Connection closed by client: %s\n", NET_AdrToStringMT(&conn->remote, adrstr, sizeof(adrstr)));
		}
		NET_TcpCloseSocket(conn->remote.sock);
		return -1;

	}else{

		conn->lastMsgTime = NET_TimeGetTime(); //Don't timeout
		return ret;
	}
}


void NET_TcpServerRebuildFDList()
{
	int 				i;
	tcpConnections_t	*conn;

	FD_ZERO(&tcpServer.fdr);
	FD_ZERO(&tcpServer.fdw);
	tcpServer.highestfd = -1;

	for(i = 0, conn = tcpServer.connections; i < MAX_TCPCONNECTIONS; i++, conn++)
	{
		if(conn->remote.sock > 0)
		{
			FD_SET(conn->remote.sock, &tcpServer.fdr);
			if(conn->wantwrite)
			{
				FD_SET(conn->remote.sock, &tcpServer.fdw);
			}
			if(conn->remote.sock > tcpServer.highestfd)
			{
				tcpServer.highestfd = conn->remote.sock;
			}
		}
	}
}

void NET_TcpServerInit(const char* privatekeyfile, const char* privatekeypassword, const char* certfiles)
{
	int 				i;
	tcpConnections_t	*conn;

	Com_Memset(&tcpServer, 0, sizeof(tcpServer));
	FD_ZERO(&tcpServer.fdr);
	FD_ZERO(&tcpServer.fdw);
	tcpServer.highestfd = -1;

	for(i = 0, conn = tcpServer.connections; i < MAX_TCPCONNECTIONS; i++, conn++)
	{
		conn->remote.sock = INVALID_SOCKET;
	}

}


/*
==================
NET_TcpServerPacketEventLoop
Only for Stream sockets (TCP)
Get new packets received on serversockets and call executing functions
==================
*/


void NET_TcpServerPacketEventLoop()
{

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	int activefd, i, ret;
	unsigned int cursize;
	fd_set fdr, fdw;
	tcpConnections_t	*conn;
	char errstr[256];
	char adrstr[256];

	byte bufData[MAX_MSGLEN];

	if(tcpServer.highestfd < 0)
	{
		// windows ain't happy when select is called without valid FDs
		return;
	}

	fdr = tcpServer.fdr;
	fdw = tcpServer.fdw;
	activefd = select(tcpServer.highestfd + 1, &fdr, &fdw, NULL, &timeout);


	if(activefd < 0)
	{
		Com_PrintWarning("NET_TcpServerPacketEventLoop: select() syscall failed: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)));
		return;
	}

	if(activefd == 0)
	{
		return;
	}
	for(i = 0, conn = tcpServer.connections; i < MAX_TCPCONNECTIONS; i++, conn++)
	{
		if(conn->remote.sock < 1)
		{
			continue;
		}
		if(FD_ISSET(conn->remote.sock, &fdr) || FD_ISSET(conn->remote.sock, &fdw))
		{
			cursize = 0;

			ret = NET_TcpServerGetPacket(conn, bufData, sizeof(bufData), qtrue);
			if(ret > 0)
			{
				cursize = ret;
			}

			if(cursize > sizeof(bufData))
			{
				Com_PrintWarning( "NET_TcpServerPacketEventLoop: Oversize packet from %s. Must not happen!\n", NET_AdrToStringMT(&conn->remote, adrstr, sizeof(adrstr)));
				cursize = sizeof(bufData);
			}
			qboolean wantwrite = NET_TCPPacketEvent(&conn->remote, bufData, cursize, &conn->connectionId, &conn->serviceId);
			if(wantwrite)
			{
				if(!conn->wantwrite)
				{
					conn->wantwrite = qtrue;
					FD_SET(conn->remote.sock, &tcpServer.fdw);
				}
			}else{
				if(conn->wantwrite)
				{
					conn->wantwrite = qfalse;
					FD_CLR(conn->remote.sock, &tcpServer.fdw);
				}
			}
			break;

		}else if(conn->lastMsgTime + MAX_TCPCONNWAITTIME < NET_TimeGetTime()){
			NET_TcpCloseSocket(conn->remote.sock);
		}
	}
}

/*
==================
NET_TcpServerOpenConnection
Only for Stream sockets (TCP)
A TCP connection request has been received #2
Find a new slot in the client array for state handling
==================
*/

void NET_TcpServerOpenConnection( netadr_t *from )
{

	tcpConnections_t	*conn;
	uint32_t			oldestTimeAccepted = 0xFFFFFFFF;
	uint32_t			oldestTime = 0xFFFFFFFF;
	int			oldestAccepted = 0;
	int			oldest = 0;
	int			i;
	char			adrstr[128];

	for(i = 0, conn = tcpServer.connections; i < MAX_TCPCONNECTIONS; i++, conn++)
	{
		if((NET_CompareBaseAdr(from, &conn->remote) && conn->state < TCP_CONNSUCCESSFULL) || conn->remote.sock < 1){//Net request from same address - Close the old not confirmed connection
			break;
		}
		if(conn->state < TCP_CONNSUCCESSFULL){
			if(conn->lastMsgTime < oldestTime){
				oldestTime = conn->lastMsgTime;
				oldest = i;
			}
		}else{
			if(conn->lastMsgTime < oldestTimeAccepted){
				oldestTimeAccepted = conn->lastMsgTime;
				oldestAccepted = i;
			}
		}
	}

	if(i == MAX_TCPCONNECTIONS)
	{
		if(tcpServer.activeConnectionCount > MAX_TCPCONNECTIONS / 3 && oldestTimeAccepted + MAX_TCPCONNECTEDTIMEOUT < NET_TimeGetTime()){
				conn = &tcpServer.connections[oldestAccepted];
				tcpServer.activeConnectionCount--; //As this connection is going to be closed decrease the counter
				NET_TCPConnectionClosed(&conn->remote, conn->connectionId, conn->serviceId);

		}else if(oldestTime + MIN_TCPCONNWAITTIME < NET_TimeGetTime()){
				conn = &tcpServer.connections[oldest];

		}else{
			closesocket(from->sock); //We have already too many open connections. Not possible to open more. Possible attack
			return;
		}
	}

	if(conn->lastMsgTime > 0)
	{
		NET_TcpCloseSocket(conn->remote.sock);
	}
	conn->remote = *from;
	conn->remote.unsentbytes = 0;
	conn->remote.security_ctx = NULL;
	conn->lastMsgTime = NET_TimeGetTime();
	conn->state = TCP_CONNWAIT;
	conn->serviceId = 0;
	conn->connectionId = 0;
	conn->wantwrite = qfalse;

	FD_SET(conn->remote.sock, &tcpServer.fdr);

	if(tcpServer.highestfd < conn->remote.sock)
		tcpServer.highestfd = conn->remote.sock;

	Com_DPrintf("Opening a new TCP server connection. Sock: %d Index: %d From: %s\n", conn->remote.sock, i, NET_AdrToStringMT(&conn->remote, adrstr, sizeof(adrstr)));

}


/*
==================
Only for Stream sockets (TCP)
A TCP connection request has been received #1
Try to accept this request
==================
*/


__optimize3 __regparm3 qboolean NET_TcpServerConnectRequest(netadr_t* net_from, fd_set *fdr){

	struct sockaddr_storage from;
	socklen_t	fromlen;
	int		conerr;
	net_from->sock = INVALID_SOCKET;
	int socket;
	ioctlarg_t	_true = 1;
	char		errstr[256];

	if(tcp_socket != INVALID_SOCKET && FD_ISSET(tcp_socket, fdr))
	{
		fromlen = sizeof(from);

		socket = accept(tcp_socket, (struct sockaddr *) &from, &fromlen);
		if (socket == SOCKET_ERROR)
		{
			conerr = socketError;

			if( conerr != EAGAIN && conerr != ECONNRESET )
				Com_PrintWarning( "NET_TcpServerConnectRequest: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );

			return qfalse;
		}
		else
		{
			if( ioctlsocket( socket, FIONBIO, &_true ) == SOCKET_ERROR ) {
				Com_PrintWarning( "NET_TcpServerConnectRequest: ioctl FIONBIO: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
				conerr = socketError;
				closesocket( socket );
				return qfalse;
			}
			SockadrToNetadr( (struct sockaddr *) &from, net_from, qtrue, socket);
			return qtrue;
		}
	}
	return qfalse;
}


#define MAX_NETPACKETS 666


__optimize3 __regparm1 qboolean NET_TcpServerConnectEvent(fd_set *fdr)
{

	netadr_t from;
	for( ; ; )
	{

		if(NET_TcpServerConnectRequest(&from, fdr))
		{

			NET_TcpServerOpenConnection( &from );

		}else{
			return qfalse;
		}
	}
	return qtrue;
}


/*========================================================================================================
Functions for TCP networking which can be used only by client
*/


/*
====================
NET_TcpClientGetData
returns number of read bytes.
Or returns: NET_WANT_READ -> Call this again soon. Nothing here yet.
Or NET_CONNRESET close the socket. Remote host aborted connection connection. [Writes readable message to errormsg if not NULL]
Or NET_ERROR close the socket. An error happened. [Writes readable message to errormsg if not NULL]
Or 0 close the socket. Action completed in most cases.
====================
*/

int NET_TcpClientGetData(int sock, unsigned char* buf, int buflen, char* errormsg, int maxerrorlen)
{

	int err;
	int ret;
	char errstr[256];

	if(sock < 1)
		return -1;

	ret = recv(sock, (char*)buf, buflen, 0);

	if(ret == SOCKET_ERROR)
	{
		err = socketError;
		if(err == EAGAIN || err == EINTR)
    	{
				return NET_WANT_READ; //Nothing more to read left or interupted system call
		}
		if(errormsg)
		{
			Q_strncpyz(errormsg, NET_ErrorStringMT(errstr, sizeof(errstr)), maxerrorlen);
		}
		if(ret == ECONNRESET || ret == EPIPE)
		{
			return NET_CONNRESET;
		}
    	return NET_ERROR;
	}
	return ret;
}

/*
====================
NET_TcpIsSocketReady
Test if socket is fully connected or not yet
====================
*/
int NET_TcpWaitForSocketIsReady(int socket, int timeoutsec)
{
	int err = 0;
	int retval;
	fd_set fdr;
	struct timeval timeout;

	FD_ZERO(&fdr);
	FD_SET(socket, &fdr);

	do{
		timeout.tv_sec = timeoutsec;
		timeout.tv_usec = 0;

		retval = select(socket +1, NULL, &fdr, NULL, &timeout);
		if(retval < 0)
		{
			err = socketError;
		}else{
			err = 0;
		}
	}while(err == EINTR);

	if(retval < 0){
		return -1; //Syscall has failed

	}else if(retval > 0){
		socklen_t so_len = sizeof(err);

		if(getsockopt(socket, SOL_SOCKET, SO_ERROR, (char*) &err, &so_len) == 0)
		{
			return 1; //Socket is connected
		}
		return -2; //Other error

	}
	return 0; //Not yet ready
}

int NET_TcpIsSocketReady(int socket) //return: 1 ready, 0 not ready, -1 select error, -2 other error
{
	return NET_TcpWaitForSocketIsReady(socket, 0);
}


/*
====================
NET_TcpClientConnect
====================
*/
int NET_TcpClientConnectInternal( netadr_t *remoteadr )
{
	SOCKET			newsocket;
	struct sockaddr_storage	address;
	char errstr[256];

	NetadrToSockadr( remoteadr, (struct sockaddr *)&address);

	if( ( newsocket = socket( address.ss_family, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET ) {
		Com_PrintWarning( "NET_TCPConnect: socket: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
		return INVALID_SOCKET;
	}

	if( connect( newsocket, (struct sockaddr *)&address, sizeof(address) ) != SOCKET_ERROR )
	{
		// make it non-blocking
		ioctlarg_t	_true = 1;
		if( ioctlsocket( newsocket, FIONBIO, &_true ) == SOCKET_ERROR ) {
			Com_PrintWarning( "NET_TCPIPSocket: ioctl FIONBIO: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
			closesocket(newsocket);
			return INVALID_SOCKET;
		}
		return newsocket;
	}

	Com_PrintWarning( "NET_TCPOpenConnection: connect error: %s\n", NET_ErrorStringMT(errstr, sizeof(errstr)) );
	closesocket( newsocket );
	return INVALID_SOCKET;

}

int NET_TcpClientConnectToAdr( netadr_t *remoteAdr )
{
  return NET_TcpClientConnectInternal(remoteAdr);
}



/*
====================
NET_Init
====================
*/
void NET_Init( qboolean isserver, unsigned int port ) {

	NET_Config( qtrue, isserver, port );
	Com_Printf("NET_Init completed\n");

}


/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void ) {
	if ( !networkingEnabled ) {
		return;
	}
	Com_Printf("---- Network shutdown ----\n");
	NET_Config( qfalse, qfalse, net_port );
	Com_Printf("--------------------------\n");
#ifdef _WIN32
	WSACleanup();
	winsockInitialized = qfalse;
#endif
}


/*
====================
NET_Sleep

Sleeps usec or until something happens on the network
====================
*/
__optimize3 __regparm1 void NET_Sleep(unsigned int usec)
{
	struct timeval timeout;
	fd_set fdr;
	int highestfd = -1;
	int retval;

	if( usec > 999999 )
		usec = 0;

	FD_ZERO(&fdr);

	if(tcp_socket != INVALID_SOCKET)
	{
		FD_SET(tcp_socket, &fdr);

		if((signed int)tcp_socket > highestfd)
		{
			highestfd = tcp_socket;
		}
	}

	timeout.tv_sec = 0;
	timeout.tv_usec = usec;

#ifdef _WIN32
	if(highestfd < 0)
	{
		// windows ain't happy when select is called without valid FDs
		SleepEx(usec / 1000, 0);
		return;
	}
#endif

	retval = select(highestfd + 1, &fdr, NULL, NULL, &timeout);

	if(retval < 0){
		if(socketError == EINTR)
		{
			return;
		}
		Com_PrintWarning("NET_Sleep: select() syscall failed: %s\n", NET_ErrorString());
		return;
	
	}else if(retval > 0){

		if((tcp_socket != INVALID_SOCKET && FD_ISSET(tcp_socket, &fdr)))
		{
			NET_TcpServerConnectEvent(&fdr);
		}

	}
}



/*
=============
NET_StringToAdr

Traps "localhost" for loopback, passes everything else to system
return 0 on address not found, 1 on address found with port, 2 on address found without port.
=============
*/
int NET_StringToAdr( const char *s, netadr_t *a, netadrtype_t family )
{
	char	base[MAX_STRING_CHARS], *search;
	char	*port = NULL;

	if (!strcmp (s, "localhost")) {
		Com_Memset (a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
// as NA_LOOPBACK doesn't require ports report port was given.
		return 1;
	}

	Q_strncpyz( base, s, sizeof( base ) );

	if(*base == '[' || Q_CountChar(base, ':') > 1)
	{
		// This is an ipv6 address, handle it specially.
		search = strchr(base, ']');
		if(search)
		{
			*search = '\0';
			search++;

			if(*search == ':')
				port = search + 1;
		}

		if(*base == '[')
			search = base + 1;
		else
			search = base;
	}
	else
	{
		// look for a port number
		port = strchr( base, ':' );

		if ( port ) {
			*port = '\0';
			port++;
		}

		search = base;
	}

	if(!Sys_StringToAdr(search, a, family))
	{
		a->type = NA_BAD;
		return 0;
	}

	if(port)
	{
		a->port = htons((short) atoi(port));
		return 1;
	}
	else
	{
		a->port = htons(PORT_SERVER);
		return 2;
	}
}

qboolean NET_IsIPv4Protocol(netadr_t* adr)
{
	int i;
	
	if(adr->type == NA_IP || adr->type == NA_TCP)
	{
		return qtrue;
	}
	if(adr->type != NA_IP6 && adr->type != NA_TCP6)
	{
		return qfalse;
	}
	for(i = 0; i < 10; ++i)
	{
		if(adr->ip6[i] != 0x00)
		{
			return qfalse;
		}
	}
	for( ; i < 12; ++i)
	{
		if(adr->ip6[i] != 0xff)
		{
			return qfalse;
		}
	}
	return qtrue;
}

