#include "network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>


boolean network_init(){
    return TRUE;
}

boolean network_exit(){
    return TRUE;
}


int network_tcp_connect_to( const char *address , const uint16_t port){
  int sockfd = -1;
  struct sockaddr_in sin;
  struct hostent *server;

  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0){
    print_error("ERROR opening socket");
    return -1;
  }
  server = gethostbyname(address);
  if (server == NULL)
  {
    print_error( hstrerror( h_errno ) );
    return -1;
  }
  memset((char *) &sin,0, sizeof(sin));
  sin.sin_family = AF_INET;
  memcpy(&sin.sin_addr,
   server->h_addr,
   server->h_length);
  sin.sin_port = htons(port);
  if (connect(sockfd,(struct sockaddr *)&sin,sizeof(sin)) < 0){
    error("ERROR connecting");
  }
  return sockfd;
}
