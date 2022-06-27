// -*- c-basic-offset: 2; indent-tabs-mode: nil -*-

/*
 *  React - Event reactor for C
 *  Copyright (C) 2001,2004-6,2012,2014,2016-7  Lancaster University
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Contact Steven Simpson <http://www.lancaster.ac.uk/~simpsons/>
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#ifdef __WIN32__
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define closesocket(S) close(S)
#endif

#include <ddslib/dllist.h>

#include "react/core.h"
#include "react/event.h"
#include "react/socket.h"
#include "react/file.h"

struct srv;

struct conn {
  struct srv *srv;
  dllist_elem(struct conn) others;
  react_sock_t sock;
  react_t datain;
  struct sockaddr_in addr;
};

struct srv {
  dllist_hdr(struct conn) conns;
  react_sock_t sock;
  react_core_t core;
  react_t acceptable;
  bool more;
};

static void display_error(const char *msg)
{
#ifdef __WIN32__
  errno = WSAGetLastError();
#endif
  perror(msg);
}

static int conn_prime(struct conn *c)
{
  return react_prime_sockin(c->datain, c->sock);
}

static void on_read(void *ctxt)
{
  struct conn *c = ctxt;
  char buf[256];
  react_ssize_t rc = recv(c->sock, buf, sizeof buf - 1, 0);
  if (rc == react_SOCKET_ERROR) {
    display_error("recv");
  } else if (rc == 0) {
    printf("%s:%d TERMINATED\n",
           inet_ntoa(c->addr.sin_addr),
           ntohs(c->addr.sin_port));
    closesocket(c->sock);
    react_close(c->datain);
    dllist_unlink(&c->srv->conns, others, c);
  } else {
    buf[rc] = '\0';
    printf("%s:%d: %s\n",
           inet_ntoa(c->addr.sin_addr),
           ntohs(c->addr.sin_port),
           buf);
    send(c->sock, buf, rc, 0);
    int rc = conn_prime(c);
    assert(rc == 0);
  }
}

static struct conn *open_conn(struct srv *srv, react_sock_t sock,
                              const struct sockaddr_in *addr)
{
  struct conn *c = malloc(sizeof *c);
  if (c == NULL) {
    perror("malloc");
    return NULL;
  }
  c->srv = srv;
  c->sock = sock;
  c->addr = *addr;
  c->datain = react_open(srv->core);
  if (c->datain == react_ERROR) {
    perror("react_open(conn)");
    free(c);
    return NULL;
  } else {
    react_direct(c->datain, &on_read, c);
    int rc = conn_prime(c);
    assert(rc == 0);
    dllist_append(&srv->conns, others, c);
    return c;
  }
}

static int srv_prime(struct srv *srv)
{
  return react_prime_sockin(srv->acceptable, srv->sock);
}

static void on_accept(void *ctxt)
{
  struct srv *srv = ctxt;
  struct sockaddr_in addr;
  react_socklen_t addrlen = sizeof addr;
  react_sock_t sock = accept(srv->sock, (struct sockaddr *) &addr, &addrlen);
  if (sock == react_INVALID_SOCKET) {
    display_error("accept");
  } else {
    struct conn *conn = open_conn(srv, sock, &addr);
    assert(conn != NULL);
  }

  int rc = srv_prime(srv);
  assert(rc == 0);
}

static int srv_init(struct srv *srv, int sock, react_core_t core)
{
  if (listen(sock, 5) < 0)
    return -1;

  srv->acceptable = react_open(core);
  if (srv->acceptable == react_ERROR)
    return -1;

  react_direct(srv->acceptable, &on_accept, srv);
  dllist_init(&srv->conns);
  srv->sock = sock;
  srv->core = core;
  srv->more = true;

  if (srv_prime(srv) < 0) {
    react_close(srv->acceptable);
    return -1;
  }
  return 0;
}

int main(int argc, const char *const *argv)
{
  printf("Welcome!\n");

#ifdef __WIN32__
  WSADATA wsaData;
  {
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
      display_error("WSAStartup");
      return EXIT_FAILURE;
    }
  }
#endif

  react_sock_t sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock == react_INVALID_SOCKET) {
    display_error("socket");
    return EXIT_FAILURE;
  }
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(8000);
  if (bind(sock, (struct sockaddr *) &addr, sizeof addr) == react_SOCKET_ERROR) {
    display_error("bind");
    return EXIT_FAILURE;
  }
  react_core_t core = react_opencore(0);
  if (core == react_COREERROR) {
    perror("react_opencore");
    return EXIT_FAILURE;
  }
  struct srv srv;
  if (srv_init(&srv, sock, core) < 0) {
    perror("srv_init");
    return EXIT_FAILURE;
  }

  printf("Polling...!\n");

  while (srv.more) {
    int rc = react_yield(core);
    if (rc < 0) {
      if (errno == EAGAIN) {
        printf("Nothing more...\n");
        break;
      }
      perror("react_yield");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
