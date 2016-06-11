#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

struct network_socket;

bool network_init();

bool network_exit();

bool network_valid_socket(const struct network_socket *sock);

struct network_socket * network_tcp_connect_to( const char *address , const uint16_t port);

int network_send( const struct network_socket *sock, const uint8_t * data, const size_t data_length );

int network_receive( const struct network_socket *sock, uint8_t * buffer, const size_t buffer_size );

void network_close( struct network_socket *sock);
#endif // NETWORK_H_INCLUDED
