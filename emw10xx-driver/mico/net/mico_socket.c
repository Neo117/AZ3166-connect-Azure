/* MiCO Team
 * Copyright (c) 2017 MXCHIP Information Tech. Co.,Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#include <string.h>
#include <stdlib.h>
#include "mico_common.h"
#include "mico_socket.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/ip_addr.h"

/******************************************************
 *                      Macros
 ******************************************************/


/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

int socket(int domain, int type, int protocol)
{
    return lwip_socket( domain, type, protocol );
}

int setsockopt (int socket, int level, int optname, void *optval, socklen_t optlen)
{
    return lwip_setsockopt( socket, level, optname, optval, optlen );
}

int getsockopt (int socket, int level, int optname, void *optval, socklen_t *optlen_ptr)
{
    return lwip_getsockopt( socket, level, optname, optval, optlen_ptr );
}

int bind (int socket, struct sockaddr *addr, socklen_t length)
{
    return lwip_bind ( socket, addr, length);
}

int connect (int socket, struct sockaddr *addr, socklen_t length)
{
    return lwip_connect( socket, addr, length );
}

int listen (int socket, int n)
{
    return lwip_listen( socket, n );
}

int accept (int socket, struct sockaddr *addr, socklen_t *length_ptr)
{
    return lwip_accept( socket, addr, length_ptr );
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    nfds = 64;

    if ((timeout->tv_sec == 0) && (timeout->tv_usec < 1000)) // timeout must bigger than 1ms.
        timeout->tv_usec = 1000;

    return lwip_select( nfds, readfds, writefds, exceptfds, timeout );
}


int send (int socket, const void *buffer, size_t size, int flags)
{
    return lwip_send( socket, buffer, size, flags );
}

int sendto (int socket, const void *buffer, size_t size, int flags, const struct sockaddr *addr, socklen_t length)
{
    return lwip_sendto( socket, buffer, size, flags, addr, length);
}

int recv (int socket, void *buffer, size_t size, int flags)
{
    return lwip_recv( socket, buffer, size, flags );
}

int recvfrom (int socket, void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t *length_ptr)
{
    return lwip_recvfrom( socket, buffer, size, flags, addr, length_ptr );
}

ssize_t read (int filedes, void *buffer, size_t size)
{
    return recv(filedes, buffer, size, 0);
}

ssize_t write (int filedes, const void *buffer, size_t size)
{
    return send(filedes, buffer, size, 0);
}

int close (int filedes)
{
    return lwip_close( filedes );
}
/*
int shutdown(int s, int how)
{
	return lwip_shutdown(s, how);
}
*/
struct hostent * gethostbyname (const char *name)
{
    return lwip_gethostbyname( name );
}

int getaddrinfo(const char *nodename,
       const char *servname,
       const struct addrinfo *hints,
       struct addrinfo **res)
{
	printf("%s, nodename %s, servname %s\r\n", __FUNCTION__, nodename, servname);
	return lwip_getaddrinfo(nodename,servname,hints,res);
}

void freeaddrinfo(struct addrinfo *ai)
{
	lwip_freeaddrinfo(ai);
}
int getpeername (int s, struct sockaddr *name, socklen_t *namelen)
{
	return lwip_getpeername (s, name, namelen);
}
int getsockname (int s, struct sockaddr *name, socklen_t *namelen)
{
	return lwip_getsockname (s, name, namelen);
}

