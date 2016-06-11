#include "network.h"

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <malloc.h>

struct network_socket{
    SOCKET s;
};


bool network_init(){
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        fprintf(stderr,"WSAStartup failed with error: %d\n", err);
        return FALSE;
    }

    return TRUE;
}

bool network_exit(){
    return TRUE;
}

bool network_valid_socket(const struct network_socket *sock){
  return sock->s != INVALID_SOCKET;
}

struct network_socket *network_tcp_connect_to( const char *address , const uint16_t port){
  struct network_socket *sock = malloc(sizeof(struct network_socket));
  //SOCKET sockfd;
  int error;
  struct sockaddr_in sin;
  struct hostent *server;

  sock->s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock->s == INVALID_SOCKET){
    error = WSAGetLastError();
    fprintf(stderr,"ERROR %d opening socket. Try NET HELPMSG %d",error,error);
    return sock;
  }
  server = gethostbyname(address);
  if (server == NULL)
  {
    error = WSAGetLastError();
    fprintf(stderr,"ERROR %d opening socket. Try NET HELPMSG %d",error,error);
    return sock;
  }

  memset((char *) &sin,0, sizeof(sin));
  sin.sin_family = AF_INET;
  memcpy(&sin.sin_addr,
   server->h_addr,
   server->h_length);
  sin.sin_port = htons(port);
  if (connect(sock->s,(struct sockaddr *)&sin,sizeof(sin)) == SOCKET_ERROR ){
    error = WSAGetLastError();
    fprintf(stderr,"ERROR %d opening socket. Try NET HELPMSG %d",error,error);
    sock->s = INVALID_SOCKET;
    return sock;
  }
  return sock;
}

int network_send( const struct network_socket *sock, const uint8_t * data, const size_t data_length ){
    return send(sock->s, (const char*)data, data_length, 0);
}

int network_receive( const struct network_socket *sock, uint8_t * buffer, const size_t buffer_size ){
    return recv(sock->s, (char*)buffer, buffer_size, 0);
}

void network_close( struct network_socket *sock){
    closesocket(sock->s);
    free(sock);
}
