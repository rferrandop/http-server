#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 100

struct request_in {
  char *method;
  char *path;
  char *version;
};

ssize_t request_line(struct request_in *request_in, char *request_header,
                     ssize_t len);

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  int server_fd, client_addr_len;
  struct sockaddr_in client_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    printf("SO_REUSEADDR failed: %s \n", strerror(errno));
    return 1;
  }

  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(4221),
      .sin_addr = {htonl(INADDR_ANY)},
  };

  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");
  client_addr_len = sizeof(client_addr);

  int client_fd =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
  if (!client_fd) {
    printf("Error creating the client socket: %s...\n", strerror(errno));
    return 1;
  }

  char *req_header = (char *)malloc(BUFFER_SIZE * sizeof(char));
  read(client_fd, req_header, BUFFER_SIZE);

  struct request_in *request;
  request = (struct request_in *)malloc(sizeof(struct request_in));

  request_line(request, req_header, strlen(req_header));

  char *message;
  printf("%s/n", request->path);
  if (strcmp(request->path, "/") == 0) {
    message = "HTTP/1.1 200 OK\r\n\r\n";
  } else {
    message = "HTTP/1.1 404 Not Found\r\n\r\n";
  }

  int send_ret = send(client_fd, message, strlen(message), 0);
  if (!send_ret) {
    printf("Error sending the response: %s...\n", strerror(errno));
    return 1;
  }
  printf("Client connected\n");

  close(server_fd);

  return 0;
}

ssize_t request_line(struct request_in *request, char *request_header,
                     ssize_t len) {
  const char spc[1] = " ";
  char *token;

  token = strtok(request_header, spc);
  request->method = (char *)malloc(strlen(token) * sizeof(char));
  strcpy(request->method, token);

  token = strtok(NULL, spc);
  request->path = (char *)malloc(strlen(token) * sizeof(char));
  strcpy(request->path, token);

  token = strtok(NULL, spc);
  request->version = (char *)malloc(strlen(token) * sizeof(char));
  strcpy(request->version, token);

  return 0;
}
