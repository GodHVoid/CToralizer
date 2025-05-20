
#include "toralizer.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
  1.2.3.4 80

*/

Req *request(const char *dstip, const int dstport) {
  Req *req;

  req = malloc(reqsize);

  req->vn = 4;
  req->cd = 1;
  req->dstport = htons(dstport);
  req->dstip = inet_addr(dstip);
  strncpy(req->userid, USERNAME, 8);

  return req;
}

int main(int argc, char *argv[]) {
  char *host;
  int port, s;

  Req *req;
  char buf[ressize];
  Res *res;

  int success;

  struct sockaddr_in sock;

  /*predicate:*/

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
    return -1;
  }

  host = argv[1];
  port = atoi(argv[2]);

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");

    return -1;
  }
  sock.sin_family = AF_INET;
  sock.sin_port = htons(PROXYPORT);
  sock.sin_addr.s_addr = inet_addr(PROXY);

  if (connect(s, (struct sockaddr *)&sock, sizeof(sock))) {
    perror("connect");
    return -1;
  }
  printf("Connected to proxy\n");
  req = request(host, port);

  // Write request struct
  if (write(s, req, reqsize) != reqsize) {
    perror("write request");
    free(req);
    close(s);
    return -1;
  }

  // Write terminating null byte for USERID
  unsigned char nullbyte = 0x00;
  if (write(s, &nullbyte, 1) != 1) {
    perror("write null terminator");
    free(req);
    close(s);
    return -1;
  }

  memset(buf, 0, ressize);

  if (read(s, buf, ressize) < 1) {
    perror("read");
    // free memory of request
    free(req);
    // close connection
    close(s);
    return -1;
  }

  res = (Res *)buf;
  success = (res->cd == 90);
  if (!success) {
    fprintf(stderr, "Error code %d:Unable to traverse the proxy\n", res->cd);
    close(s);
    free(req);
    return -1;
  }
  printf("Successfully connected through the proxy to %s:%d\n", host, port);
  close(s);
  free(req);
  return 0;
}
