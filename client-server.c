/*
** client-server.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>


#include "circular_buffer.h"
#include "./peer/peer.h"



#define PORTC                    "9035" // the port client will be connecting to
#define PORTS                    "9034" // the port server will be listening to

#define BUFFER_SIZE             276
#define CIRCULAR_BUFFER_SIZE    2000
#define TOTAL_MESSAGES_RECV     2000000 // 2.000.000

FILE *fd_find_msg_time;
uint64_t total_messages_recv = 0;

typedef struct client_data{
  char **servIP;
  int servIP_size;
  char *myip;
  char *myaem;
  cbuf_handle_t cbuf;
}client_data;

struct message {
	char msg[276]; // uint32_t + uint32_t + uint64_t + char[256] + 3 * "_" = 276 bytes
	uint64_t timestamp;
};



char *aemtoIP(uint32_t aem);
message create_message(uint32_t from, uint32_t to, char *txt);
void print_buffer_status(cbuf_handle_t cbuf);
void print_circular_buffer(cbuf_handle_t cbuf);
void save_data_to_circular_buffer(cbuf_handle_t cbuf, char *data, char *myaem);

void *start_client(void *cl_data);
int client_connect(struct addrinfo *servinfo); // returns a sockfd

void *start_server(void *server_data);
int server_bind(struct addrinfo *ai);
int send_to_peer(peer_t *peer, cbuf_handle_t cbuf);


int main(int argc, char const *argv[])
{
  pthread_t client_tid, server_tid;
  client_data cl_data;
  char **ip;
  struct timeval tv;

  //=============== Create circular buffer ===============
  message * buffer  = malloc(CIRCULAR_BUFFER_SIZE * sizeof(message));

  printf("\n=== C Circular Buffer Check ===\n");

  cbuf_handle_t cbuf = circular_buf_init(buffer, CIRCULAR_BUFFER_SIZE);
  //=======================================================



  // //================= Add some values =======================
  // printf("\nAdding %d values\n", 10);
  // printf("TEXT \t\t\t\t, TIMESTAMP, \t\t SIZE\n");
  //
  // message msg;
  //
	// for(size_t i = 0; i < 10; i++) {
  //   gettimeofday(&tv, NULL);
  //
  //   msg.timestamp = tv.tv_sec * 1000000 + tv.tv_usec;
  //
	// 	sprintf(msg.msg, "%s%" PRIu64, "8701_8703_MYSTRING_", msg.timestamp);
  //
  //   circular_buf_push(cbuf, msg);
  //
  //   printf("Added %s, %" PRIu64", Size now: %zu\n", msg.msg, msg.timestamp, circular_buf_size(cbuf));
  // }
  // printf("head: %zu , tail: %zu\n", circular_buf_get_head(cbuf),
  //                                   circular_buf_get_tail(cbuf));
  //
  // printf("Circular Buffer initialized. \n");
  //
	// print_buffer_status(cbuf);
  // printf("Circular Buffer: head: %zu , tail: %zu\n", circular_buf_get_head(cbuf),
  //                                                   circular_buf_get_tail(cbuf));
  // //=======================================================



  //========================= client data ==============================
  cl_data.cbuf = cbuf;

  cl_data.myip = "10.0.87.1";
  printf("my IP is: %s\n", cl_data.myip);
  cl_data.myaem = "8701";
  printf("my AEM is: %s\n", cl_data.myaem);

  // Load data
  uint32_t *aems = malloc(2 * sizeof(uint32_t));
  aems[0] = 8700;
  // aems[1] = 8701;
  // aems[2] = 8702;
  // aems[3] = 8703;
  // aems[4] = 8704;


  ip = malloc(6 * sizeof(char*));
  for (size_t i = 0; i < 1; i++) {
    ip[i] = aemtoIP(aems[i]);
  }
  ip[1] = strdup("192.168.0.1");
  // ip = malloc(5 * sizeof(char*));
  // if (ip == NULL) exit(1);
  // ip[0] = strdup("DESKTOP-3181HB5");
  cl_data.servIP = ip;
  cl_data.servIP_size = 2;

  for (size_t i = 0; i < cl_data.servIP_size; i++) {
    printf("cl_data.servIP: %s\n", cl_data.servIP[i]);
  }
  //===========================================================================



  //========================= Server Data ===================================
  peer_t *client_list;
  size_t cl_list_size;
  void* server_data[3];

  cl_list_size = cl_data.servIP_size;
  client_list = (peer_t*)malloc(cl_list_size * sizeof(peer_t));
  if (client_list == NULL) {
    fprintf(stderr, "Cant alocate memory for 'client_list'\n");
    exit(1);
  }
  for (size_t i = 0; i < cl_list_size; i++) {
    init_peer(&client_list[i], cl_data.servIP[i]);
    print_peer(&client_list[i]);
  }

  server_data[0] = (void*)client_list;
  server_data[1] = (void*)&cl_list_size;
  server_data[2] = (void*)cbuf;
  //===========================================================================



//========== start server, client and message creator =============
  fd_find_msg_time = fopen("find_msg_time.txt", "w");
  if (fd_find_msg_time == NULL)
  {
      fprintf(stderr, "Error opening file!\n");
      exit(1);
  }
  pthread_create(&server_tid, NULL, start_server, (void*)server_data);

  pthread_create(&client_tid, NULL, start_client, (void*)&cl_data);

  srandom(time(NULL));
  message newmsg;
  char txt[256];


  for (size_t i = 0;; i++) {
    sprintf(txt, "test-message-No-%zu", i);
    newmsg = create_message(random()%2000 + 8000, random()%2000 + 8000, txt);
    if (strcmp(newmsg.msg, "Too_long") == 0) {
      continue;
    }

    gettimeofday(&tv, NULL);
    newmsg.timestamp = tv.tv_sec * 1000000 + tv.tv_usec;

    circular_buf_push(cbuf, newmsg);
    // usleep((random()%240000000 + 60000000)); // sleep for 60 - 300 seconds
    // sleep(2);

    // print_buffer_status(cbuf);
    if (total_messages_recv > TOTAL_MESSAGES_RECV) {
      break;
    }
  }

  pthread_join(server_tid, NULL);
  pthread_join(client_tid, NULL);

  print_circular_buffer(cl_data.cbuf);
  print_buffer_status(cbuf);

  fclose(fd_find_msg_time);

  //=================================================================



  //==================== Free Memory Alocations ====================
  free(aems);
  for (size_t i = 0; i < cl_data.servIP_size; i++) {
    free(cl_data.servIP[i]);
  }
  free(cl_data.servIP);
  free(buffer);
	circular_buf_free(cbuf);
  //=================================================================


  return 0;
}



//========== Helping Functions ======================
message create_message(uint32_t from, uint32_t to, char *txt)
{
  struct timeval tv;
  message msg;
  uint64_t timestamp;

  if (strlen(txt) > 255) {
    fprintf(stderr, "create_message: message is too long\n");
    strcpy(msg.msg, "Too_long");
    return msg;
  }

  gettimeofday(&tv, NULL);
  timestamp = tv.tv_sec * 1000000 + tv.tv_usec;

  sprintf(msg.msg, "%" PRIu32"_%" PRIu32"_%" PRIu64"_%s-END", from, to, timestamp, txt);

  return msg;
}

char *aemtoIP(uint32_t aem)
{
  char *ip = malloc(12 * sizeof(char));
  uint32_t xx, yy;

  xx = aem / 100;
  yy = aem - xx * 100;

  printf("10.0.%" PRIu32 ".%" PRIu32 "\n", xx, yy);
  sprintf(ip, "10.0.%" PRIu32 ".%" PRIu32, xx, yy);

  return ip;
}

void save_data_to_circular_buffer(cbuf_handle_t cbuf, char *data, char *myaem)
{
  message msg;
  char aem_rcvr[5];
  struct timeval tv, tv_start, tv_end;
  int rv;

  strncpy(aem_rcvr, data+5, 4);

  if (strcmp(aem_rcvr, myaem) != 0) {
    gettimeofday(&tv_start, NULL);
    rv = circular_buffer_find_msg(cbuf, data);
    gettimeofday(&tv_end, NULL);

    fprintf(fd_find_msg_time, "duration(usec): %lu\n",
                                      (tv_end.tv_sec * 1000000 + tv_end.tv_usec)
                                     -(tv_start.tv_sec * 1000000 + tv_start.tv_usec));

    if (rv == -1) { // data is not duplicate
      // printf("client: message '%s' was saved\n", data);


      strcpy(msg.msg, data);
      // printf("client: Recieved message:%s\n", msg.msg);

      gettimeofday(&tv, NULL);
      msg.timestamp = tv.tv_sec * 1000000 + tv.tv_usec;

      circular_buf_push(cbuf, msg);
    } else if (rv != -1) {
      printf("client: message '%s' duplicate\n", data);
    }
  } else {
    printf("client: reciever is me: '%s'\n" \
            "cliend: message '%s' was not saved in circular buffer\n", aem_rcvr, data);
  }
}

void print_buffer_status(cbuf_handle_t cbuf)
{
	printf("\nCircular buffer: Full: %d, empty: %d, size: %zu\n\n",
		circular_buf_full(cbuf),
		circular_buf_empty(cbuf),
		circular_buf_size(cbuf));
}

void print_circular_buffer(cbuf_handle_t cbuf)
{
  message msg;

  printf("\n**************************\n" \
           "   Reading back values:\n   " \
           "**************************\n");
	while(!circular_buf_empty(cbuf))
	{

		circular_buf_pop(cbuf, &msg);
		printf("%s, %" PRIu64"\n",msg.msg, msg.timestamp);
	}
	printf("\n");
}


//========== Client Functions ======================
void *start_client(void *cl_data)
{
	int sockfd, numbytes;
	char buf[BUFFER_SIZE];
	struct addrinfo hints, *servinfo;
	int rv;
  FILE *fd_save_msg_time;
  struct timeval tv_start, tv_end;

  fd_save_msg_time = fopen("save_msg_time.txt", "w");
  if (fd_save_msg_time == NULL)
  {
      fprintf(stderr, "Error opening file!\n");
      exit(1);
  }

  while (1) { // start again
    for (size_t i = 0; i < ((client_data*)cl_data)->servIP_size; i++) { // loop through ip address
      if (strcmp(((client_data*)cl_data)->servIP[i], ((client_data*)cl_data)->myip) == 0) {
        // printf("found my ip '%s'\n", ((client_data*)cl_data)->servIP[i]);
        continue;
      }
      printf("client: connecting to '%s'\n", ((client_data*)cl_data)->servIP[i]);

    	memset(&hints, 0, sizeof hints);
    	hints.ai_family = AF_INET;
    	hints.ai_socktype = SOCK_STREAM;

    	if ((rv = getaddrinfo(((client_data*)cl_data)->servIP[i], PORTC, &hints, &servinfo)) != 0) {
    		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    		exit(1);
    	}

      if ((sockfd = client_connect(servinfo)) == -1) {   // if client is unable to connect
        continue;                                      // try with the next IP
      }

    	freeaddrinfo(servinfo); // all done with this structure

      do {
        if ((numbytes = recv(sockfd, buf, BUFFER_SIZE-1, 0)) == -1) {
      		perror("recv");
      		exit(1);
        }

      	buf[numbytes] = '\0';

      	// printf("client: received '%s'(%d bytes)\n", buf, numbytes);

        if (numbytes == BUFFER_SIZE-1) { // save data only if are fully sent
          total_messages_recv++;
          gettimeofday(&tv_start, NULL);
          save_data_to_circular_buffer(  ((client_data*)cl_data)->cbuf,
                                        buf,
                                        ((client_data*)cl_data)->myaem);
          gettimeofday(&tv_end, NULL);
          fprintf(fd_save_msg_time, "duration(usec): %lu\n",
                                            (tv_end.tv_sec * 1000000 + tv_end.tv_usec)
                                           -(tv_start.tv_sec * 1000000 + tv_start.tv_usec));
        }
      } while(numbytes > 0);

    	close(sockfd);
      // pthread_exit(0);
    } // END looping through ip address
    if (total_messages_recv > TOTAL_MESSAGES_RECV) {
      break;
    }
    printf("%"PRIu64"\n", total_messages_recv);
  } // END while(1)

  fclose(fd_save_msg_time);
  pthread_exit(0);
}

int client_connect(struct addrinfo *servinfo)
{
  int sockfd;
  struct addrinfo *p;
  char servIP[INET_ADDRSTRLEN];

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("client: connect");
      close(sockfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return -1;
  }

	inet_ntop(p->ai_family, &p->ai_addr, servIP, sizeof servIP);
	printf("client: connecting to '%s'\n", servIP);

  return sockfd;
}


//========== Server Functions ======================
void *start_server(void *server_data)
{
  peer_t *peerptr;
  peer_t *peer_list = (peer_t*)((void**)server_data)[0];
  size_t pl_size = *( (size_t*)( (void**)server_data )[1] );
  cbuf_handle_t cbuf = (cbuf_handle_t)((void**)server_data)[2];

  fd_set master_r;    // master_r file descriptor list
  fd_set master_w;    // master_w file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  fd_set write_fds; // temp file descriptor list for select()
  int fdmax;        // maximum file descriptor number

  int listener;     // listening socket descriptor
  int newfd;        // newly accept()ed socket descriptor
  struct sockaddr_in remoteaddr; // client address
  socklen_t addrlen;

  char buf[BUFFER_SIZE];    // buffer for client data
  int nbytes;

	char remoteIP[INET_ADDRSTRLEN];

  int i, rv;

	struct addrinfo hints, *ai;

  FD_ZERO(&master_r);    // clear the master_r and temp sets
  FD_ZERO(&master_w);
  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORTS, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}

  if ((listener = server_bind(ai)) == -1) {
    fprintf(stderr, "selectserver: failed to bind\n");
    exit(2);
  }


	freeaddrinfo(ai); // all done with this

  // listen
  if (listen(listener, 10) == -1) {
      perror("listen");
      exit(3);
  }

  // add the listener to the master_r set
  FD_SET(listener, &master_r);

  // keep track of the biggest file descriptor
  fdmax = listener; // so far, it's this one

  // main loop
  for(;;) {
    read_fds = master_r; // copy read set
    write_fds = master_w; // copy write set
    if (select(fdmax+1, &read_fds, &write_fds, NULL, NULL) == -1) {
      perror("select");
      exit(4);
    }

    // run through the existing connections looking for data to read
    for(i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &write_fds)) { // we got ready to write
        memset(buf,'\0', BUFFER_SIZE);
        // sprintf(buf, "%d: %s", i, "My first send!");
        peerptr = find_peer_of_socket(peer_list, pl_size, i);

        // sleep(2);
        if ((rv = send_to_peer(peerptr, cbuf)) == -1) {
          fprintf(stderr, "send_to_peer: failed to send to '%s%d'\n", peerptr->ipv4,
                                                                      peerptr->socket);
        } else if (rv == -2) {
          // printf("server: All messages have been sent sent to '%s:%d'\n", peerptr->ipv4,
                                                                          // peerptr->socket);
        }

        // fflush(NULL);
      }

      if (FD_ISSET(i, &read_fds)) { // we got one on read!!
        if (i == listener) {
          // handle new connections
          addrlen = sizeof remoteaddr;
          newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

          if (newfd == -1) {
            perror("accept");
          } else {


            printf("server: new connection from '%s' on "
                "socket '%d'\n",
                                        inet_ntop(remoteaddr.sin_family,
                                                &remoteaddr.sin_addr,
                                                remoteIP, INET_ADDRSTRLEN),
                                        newfd);

            peerptr = find_peer_of_ipv4(peer_list, pl_size, remoteIP);

            if (peerptr == NULL) {
              printf("server: peer '%s:%d' is not in the list\n", remoteIP, newfd);
              printf("server: peer '%s:%d' was disconnected\n", remoteIP, newfd);
                     close(newfd);
            } else {
              FD_SET(newfd, &master_r); // add to master_r set
              FD_SET(newfd, &master_w); // add to master_w set

              if (newfd > fdmax) {    // keep track of the max
                fdmax = newfd;
              }
              peerptr->socket = newfd;
              print_peer(peerptr);
            }
          }
        } else {
          // handle data from a client
          if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) { // got error or connection closed by client

            peerptr = find_peer_of_socket(peer_list, pl_size, i);

            if (peerptr == NULL) {
              fprintf(stderr, "Can't locate peer on socket '%d'\n", i);

              if (nbytes == 0) {// connection closed
                printf("server: connection '%d' hung up\n", i);
              } else {
                perror("recv");
              }
            } else {
              if (nbytes == 0) {// connection closed
                printf("server: connection '%s:%d' hung up\n",peerptr->ipv4, peerptr->socket);
              } else {
                perror("recv");
              }

              disconnect_peer(peerptr);
              print_peer(peerptr);
            }

            close(i); // bye!
            FD_CLR(i, &master_r); // remove from master_r set
            FD_CLR(i, &master_w); // remove from master_w set
          }
        } // END handle data from client
      } // END got new incoming connection
    } // END looping through file descriptors
    if (total_messages_recv > TOTAL_MESSAGES_RECV) {
      break;
    }
    printf("%"PRIu64"\n", total_messages_recv);
  } // END for(;;)

  pthread_exit(0);
}

int server_bind(struct addrinfo *ai)
{
  struct addrinfo *p;
  int yes=1;        // for setsockopt() SO_REUSEADDR, below
  int listener;     // listening socket descriptor

  for(p = ai; p != NULL; p = p->ai_next) {
    listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (listener < 0) {
			continue;
		}

		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		return -1;
	}

  return listener;
}

int send_to_peer(peer_t *peer, cbuf_handle_t cbuf)
{
  message msg_to_send, msg_temp;
  size_t msg_to_send_pos;
  char buf[BUFFER_SIZE];
  int nbytes;

  msg_to_send.timestamp = 0;

  // print_peer(peer);

  if (circular_buf_empty(cbuf)) {
    fprintf(stderr, "send_to_peer: Circular Buffer is empty\n");
    return -3;
  }

  if (circular_buf_read(cbuf, peer->last_position, &msg_temp) == -1) {
    fprintf(stderr, "send_to_peer: circular_buf_read: failed to read peer last_position\n");
    return -1;
  }

  if (peer->last_timestamp == msg_temp.timestamp) {
    if (peer->last_position != circular_buf_get_head(cbuf)) { // check if all messages are sent
      msg_to_send_pos = circular_buf_next_pos(cbuf, peer->last_position);

      if (circular_buf_read(cbuf, msg_to_send_pos, &msg_to_send) == -1) {
        fprintf(stderr, "send_to_peer: circular_buf_read: failed to read msg_to_send_pos(1)\n");
        return -1;
      }
    }
  } else {
    msg_to_send_pos = circular_buf_get_tail(cbuf);

    if (circular_buf_read(cbuf, msg_to_send_pos, &msg_to_send) == -1) {
      fprintf(stderr, "send_to_peer: circular_buf_read: failed to read msg_to_send_pos(2)\n");
      return -1;
    }
  }

  if (msg_to_send.timestamp > 0) {
    memset(buf,'\0', BUFFER_SIZE);
    strcpy(buf, msg_to_send.msg);

    if ((nbytes = send(peer->socket, buf, BUFFER_SIZE-1, 0)) == -1) {
      perror("send");
    } else if (nbytes == BUFFER_SIZE-1) {
      peer->last_timestamp = msg_to_send.timestamp;  // save timestamp of the last message sent
      peer->last_position = msg_to_send_pos; // save the possition of the last message sent

      return 0;
    }
  }

  return -2;
}
