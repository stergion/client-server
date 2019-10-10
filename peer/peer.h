#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>
#include <assert.h>

typedef struct {
  char ipv4[INET_ADDRSTRLEN];
  int socket;

  /* the position in the buffer of the last message sent to the peer */
  size_t last_position;
  /* the timestamp in the last_position  */
  uint64_t last_timestamp;
} peer_t;

void init_peer(peer_t *peer, char *ipv4)
{
  assert(peer && ipv4);

  strcpy(peer->ipv4, ipv4);
  peer->socket = -1;
  peer->last_position = 0;
  peer->last_timestamp = 0;
}

peer_t* find_peer_of_ipv4(peer_t *peer_list, size_t list_size, char *ip)
{
  assert(peer_list && list_size);

  for (size_t i = 0; i < list_size; i++) {
    if (strcmp(peer_list[i].ipv4, ip) == 0) {
      return &peer_list[i];
    }
  }

  return NULL;
}

peer_t* find_peer_of_socket(peer_t *peer_list, size_t list_size, int socket)
{
  assert(peer_list && list_size);

  for (size_t i = 0; i < list_size; i++) {
    if (peer_list[i].socket == socket) {
      return &peer_list[i];
    }
  }

  return NULL;
}
void print_peer(peer_t *peer)
{
  assert(peer);

  printf("ipv4: %s \t socket: %d \t last_position: %zu \t last_timestamp:"\
                                          " %" PRIu64"\n", peer->ipv4,
                                                          peer->socket,
                                                          peer->last_position,
                                                          peer->last_timestamp);
}

void disconnect_peer(peer_t *peer)
{
  assert(peer);

  peer->socket = -1;
}
