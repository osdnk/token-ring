#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>


char *multicast_ip_address;
int multicast_port;
int socket_for_sending;
int multicast_socket;
int socket_for_receiving;
int tcp_in_socket;

char *identyfier_of_user;
int my_port;
char *next_ip_in_ring;
int next_port_in_ring;
int should_start_with_token;
char *choosen_protocol;

typedef enum {
    NEW_USER,
    MESSAGE,
} MessageType;

typedef struct {
    MessageType msg_type;
    int value;
    int port;
    int next_port;
} Token;


void udp_init_input_socket();
void udp_init_output_socket();
void udp_send_token(Token token);
void udp_send_init_token();
void tcp_init_input_socket();
void tcp_init_output_socket();
void tcp_accept();
void tcp_connect();
void tcp_send_token(Token token);
void tcp_send_init_token();



char *multicast_ip_address = "224.0.0.1";
int multicast_port = 9010;

void init_multicast() {
  if ((multicast_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    printf("Error during init_multicast()\n");
    exit(1);
  }
}

void send_multicast(char *message, size_t size) {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(multicast_ip_address);
  addr.sin_port = htons(multicast_port);

  if (sendto(multicast_socket, message, size, 0, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    printf("Error during multicast send\n");
    exit(1);
  }
}

int main(int argc, char **argv) {
  if (argc != 7) {
    printf("Invalid arguments' number.\n");
    return 1;
  }

  // parsing cofiguration
  identyfier_of_user = argv[1];
  my_port = atoi(argv[2]);
  next_ip_in_ring = argv[3];
  next_port_in_ring = atoi(argv[4]);
  should_start_with_token = atoi(argv[5]);
  choosen_protocol = argv[6];

  // verifying whether arguments are fine
  if (should_start_with_token != 0 && should_start_with_token != 1) {
    printf("should_start_with_token should be 0 or 1 for false or true respectively \n");
    return 1;
  }

  // verifying whether arguments are fine
  if (strcmp(choosen_protocol, "tcp") != 0 && strcmp(choosen_protocol, "udp") != 0) {
    printf("Should be udp or tcp\n");
    return 1;
  }

  // And printing configuration
  printf("CLIENT\n");
  printf("--------------------\n");
  printf("identyfier_of_user:      %s\n"
         "my_port:                 %d\n"
         "next_ip_in_ring:         %s\n"
         "next_port_in_ring:       %d\n"
         "should_start_with_token: %d\n"
         "choosen_protocol:        %s\n",
   identyfier_of_user, my_port, next_ip_in_ring, next_port_in_ring, should_start_with_token, choosen_protocol);

  // -------- end of config ----------


  // multicast is UDP
  init_multicast();

  // At the very begining we're making initalazation of sockets
  if (strcmp(choosen_protocol, "tcp") == 0) {
    if (should_start_with_token == 1) {
      Token token;

      // init socket for receiving
      tcp_init_input_socket();
      // and sending as well
      tcp_init_output_socket();
      // accepting means connecting with oneself
      tcp_accept();

      // and waiting for someone to send init token to me
      read(tcp_in_socket, &token, sizeof(token));
      // assuming it's initialization token ne send next port to it
      next_port_in_ring = token.port;
      // and start sending messages
      token.msg_type = MESSAGE;
      // which are random numbers
      token.value = rand() % 437387;
      // informing
      printf("Send %d\n", token.value);
      // and actually sending
      tcp_send_token(token);
      //should_start_with_token = 0;
    } else {
      // if we are not first and not producing tokens
      tcp_init_input_socket();
      tcp_init_output_socket();
      // on sending we also closing sockets
      // Lemme explain. I do it in order to handle adding new
      // clients. We have to listen to every new trial of connection
      // not only focus on existing one
      tcp_send_init_token();
      printf("Sent INIT token\n");
    }
  } else if (strcmp(choosen_protocol, "udp") == 0) {
    // init socker for receiving
    udp_init_input_socket();
    // and sending
    udp_init_output_socket();
    // and say hello
    udp_send_init_token();

    if (should_start_with_token == 1) {
      // then generate token if needed
      Token token;
      // as message
      token.msg_type = MESSAGE;
      // with some random value
      token.value = rand() % 123123;
      // and send it
      udp_send_token(token);
      printf("Send %d\n", token.value);
    }
  }


  // then if TCP
  if (strcmp(choosen_protocol, "tcp") == 0) {
    // make network loop
    while (1) {
      Token token;
      // accept whatever it is
      tcp_accept();
      // and read it
      read(tcp_in_socket, &token, sizeof(token));

      switch (token.msg_type) {
        // if it's new user
        case NEW_USER:
          // then remap
          printf("Received new_user token\n");
              printf("Got %d\n", token.next_port);

              if (next_port_in_ring == token.next_port) {
            printf("Swithicng ports\n");
            next_port_in_ring = token.port;
          } else {
            // if remaping is not your case send it elsewhere
            tcp_send_token(token);
          }
          break;
          // and if it's a normal message
        case MESSAGE:
          // send it to clients
          send_multicast(identyfier_of_user, strlen(identyfier_of_user) * sizeof(char));
          printf("Received %d\n", token.value);
          token.value = rand() % 245225;
          printf("Sending %d\n", token.value);
          sleep(1);
          tcp_send_token(token);
      }
    }
  } else {
    // and if UDP respectively
    while (1) {
      // definer var
      Token token;
      // and receive message
      // TODO figure our last arguments
      recvfrom(socket_for_receiving, &token, sizeof(token), 0, NULL, NULL);
      switch (token.msg_type) {
        // if new user says hello
        case NEW_USER:
          printf("new_user token\n");
          if (next_port_in_ring == token.next_port) {
            // then remap ports if needed
            printf("switching ports\n");
            next_port_in_ring = token.port;
          } else
            // else send it somewhere else
            udp_send_token(token);
          break;
        // and receiving message
        case MESSAGE:
          // inform to loggers
          send_multicast(identyfier_of_user, strlen(identyfier_of_user) * sizeof(char));
          // and pring received value
          printf("Received %d\n", token.value);
          //should_start_with_token = 1;
          // wait
          sleep(1);
          // generate new one
          token.value = rand() % 245225;
          // and send it somewhere
          printf("Sending %d\n", token.value);
          udp_send_token(token);
          //should_start_with_token = 0;
      }
    }
  }
}


/// UDP LOGIC

void udp_init_input_socket() {
  if ((socket_for_receiving = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    exit(1);
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(my_port);

  if (bind(socket_for_receiving, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    exit(1);
  }
}

void udp_init_output_socket() {
  if ((socket_for_sending = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    exit(1);
  }
}

void udp_send_token(Token token) {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(next_ip_in_ring);
  addr.sin_port = htons(next_port_in_ring);

  if (sendto(socket_for_sending, &token, sizeof(token), 0, (const struct sockaddr *) &addr, sizeof(addr)) != sizeof(token)) {
    exit(1);
  }
}

void udp_send_init_token() {
  Token token;
  token.msg_type = NEW_USER;
  token.next_port = next_port_in_ring;
  token.port = my_port;
  udp_send_token(token);
}


/// TCP LOGIC

void tcp_init_input_socket() {
  if ((socket_for_receiving = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    exit(1);
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(my_port);

  if (bind(socket_for_receiving, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
    exit(1);
  }

  if (listen(socket_for_receiving, 10) != 0) {
    exit(1);
  }
}

void tcp_init_output_socket() {
  if ((socket_for_sending = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    exit(1);
  }
}

void tcp_accept() {
  if ((tcp_in_socket = accept(socket_for_receiving, NULL, NULL)) == -1) {
    exit(1);
  }
}


void tcp_send_token(Token token) {
  // it's important to make new socket for each connection
  // since new clients may appear
  tcp_init_output_socket();
  tcp_connect();
  write(socket_for_sending, &token, sizeof(token));
  close(socket_for_sending);
}

void tcp_send_init_token() {
  Token token;
  token.msg_type = NEW_USER;
  token.next_port = next_port_in_ring;
  token.port = my_port;

  tcp_send_token(token);
}


void tcp_connect() {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(next_port_in_ring);

  if (connect(socket_for_sending, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
    printf("Error during tcp connect\n");
    exit(1);
  }
}
