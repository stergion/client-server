#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>


#include "peer.h"

#define LSIZE   20

void myfunc(void *var) {
  peer_t *peer_list = (peer_t*)((void**)var)[0];
  // size_t *pl_size_ptr = ((void**)var)[1];
  size_t pl_size = *( (size_t*)( (void**)var )[1] );
  peer_t *peer;
  for (size_t i = 0; i < pl_size; i++) {
    printf("ipv4: %s \t socket: %d \t last_position: %zd \t last_timestamp: %" PRIu64"\n",
                                                                  peer_list[i].ipv4,
                                                                  peer_list[i].socket,
                                                                  peer_list[i].last_position,
                                                                  peer_list[i].last_timestamp);
  }


  if ((peer = find_peer_of_ipv4(peer_list, pl_size, "10.0.87.1")) != NULL) {
    printf("\n\nPeer found\n");

    disconnect_peer(NULL);

    printf("ipv4: %s \t socket: %d \t last_position: %zd \t last_timestamp: %" PRIu64"\n",
                                                                  peer->ipv4,
                                                                  peer->socket,
                                                                  peer->last_position,
                                                                  peer->last_timestamp);
  } else {
    printf("Not found\n");
  }

  if ((peer = find_peer_of_socket(peer_list, pl_size, 5)) != NULL) {
    printf("\n\nPeer found\n");
    print_peer(peer);
  } else {
    printf("Not found\n");
  }
}
int main(int argc, char const *argv[]) {
  peer_t *peer_list = malloc(LSIZE * sizeof(peer_t));
  char str[16];
  size_t pl_size = LSIZE;
  void* server_data[2];

  for (size_t i = 0; i < LSIZE; i++) {
    sprintf(str, "10.0.87.%zu", i);
    init_peer(&peer_list[i], str);
  }

  server_data[0] = (void*)peer_list;
  server_data[1] = (void*)&pl_size;
  myfunc((void*)server_data);

  free(peer_list);
  return 0;
}
