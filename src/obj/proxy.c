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
 *  Contact Steven Simpson <https://github.com/simpsonst>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>

#ifdef __WIN32__
#include <winsock2.h>
#include <windows.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include <ddslib/dllist.h>

#include "react/core.h"
#include "react/user.h"
#include "react/socket.h"

#if __STDC_VERSION__ < 199901L
#error "C99 required"
#endif

#if defined __WIN32__
#elif defined __riscos || defined __riscos__
#define closesocket(X) close(X)
#define ioctlsocket(X,Y,Z) ioctl(X,Y,Z)
#else
#define closesocket(X) close(X)
#define ioctlsocket(X,Y,Z) ioctl(X,Y,Z)
#endif

#define BUFFER_SIZE 128

struct conn {
  dllist_elem(struct conn) others;

  struct side {
    char label[20];
    react_sock_t sock;
    react_t ready;
    react_iomode_t lastmode;
    unsigned char buf[BUFFER_SIZE];
    size_t cap, mark;
    unsigned can_write : 1, terminated : 1, nonblocking : 1;
  } upstream, downstream;

  struct proxy *proxy;
};

struct proxy {
  react_sock_t server;
  react_t accepting;

  dllist_hdr(struct conn) conns;
  react_core_t core;

  struct sockaddr_in target;
};

void close_conn(struct conn *conn)
{
  react_close(conn->upstream.ready);
  react_close(conn->downstream.ready);
  closesocket(conn->downstream.sock);
  closesocket(conn->upstream.sock);
  dllist_unlink(&conn->proxy->conns, others, conn);
  free(conn);
}

static void dump(const void *vstart, size_t len)
{
  const unsigned char *s = vstart;
  size_t p = 0;

  while (p < len) {
    size_t i;

    for (i = 0; i < 8; i++) {
      if (p + i >= len)
        printf("   ");
      else
        printf(" %2x", (unsigned) s[p + i]);
    }

    for (i = 0; i < 8; i++) {
      if (p + i >= len)
        printf("    ");
      else if (isprint(s[p + i]))
        printf(" %3c", s[p + i]);
      else
        printf(" %03u", (unsigned) s[p + i]);
    }

    putchar('\n');
    p += 8;
  }
}

static int handle_connected(struct conn *conn)
{
  if (connect(conn->upstream.sock, (struct sockaddr *) &conn->proxy->target,
              sizeof conn->proxy->target) == react_SOCKET_ERROR)
    return 0;
  conn->upstream.can_write = 1;
  printf("We are connected to server.\n");
  return 1;
}

static int handle_send(struct conn *conn, struct side *in, struct side *out)
{
  if (in->mark < in->cap) {
    printf("\nsending to %s: [%lu, %lu)\n", out->label,
           (unsigned long) in->mark, (unsigned long) in->cap);

    /* Try to send stored data now. */
    int done = send(out->sock,
                    (void *) (in->buf + in->mark),
                    in->cap - in->mark, 0);
    if (done == react_SOCKET_ERROR) {
      if (out->nonblocking &&
#ifdef __WIN32__
          WSAGetLastError() != WSAEWOULDBLOCK
#else
          errno != EAGAIN
#endif
          )
        return 0;
      done = 0;
      out->can_write = 0;
    } else {
#ifdef __WIN32__
      /* We don't get another write signal unless the send() call
         failed. */
      printf("\tstill can write to %s\n", out->label);
#else
      /* It's safe in Unix - select() will still say we can write. */
      out->can_write = 0;
#endif
    }

    in->mark += done;

    /* If we've sent all of our data, we can start at the beginning of
       the buffer again. */
    if (in->mark == in->cap)
      in->mark = in->cap = 0;
    printf("       [%lu, %lu)\n",
           (unsigned long) in->mark, (unsigned long) in->cap);
  } else {
    printf("\ncan write to %s now\n", out->label);
    out->can_write = 1;
  }

  return 1;
}

static int handle_recv(struct conn *conn, struct side *in, struct side *out)
{
  printf("\nreceiving from %s: [%lu, %lu)\n", in->label,
         (unsigned long) in->mark, (unsigned long) in->cap);
  int done = recv(in->sock,
                  (void *) (in->buf + in->cap),
                  BUFFER_SIZE - in->cap, 0);
  if (done == react_SOCKET_ERROR)
    return 0;

  if (done == 0) {
    /* No more data. */
    in->terminated = 1;
    printf("  Terminated\n");
  } else {
    dump(in->buf + in->cap, done);
    in->cap += done;
    printf("       [%lu, %lu)\n",
           (unsigned long) in->mark, (unsigned long) in->cap);
  }
  if (out->can_write)
    return handle_send(conn, in, out);
  return 1;
}

static int prime(struct conn *conn, react_iomode_t mode,
                 react_proc_t *pr, struct side *side)
{
  react_direct(side->ready, pr, conn);
  if (react_isprimed(side->ready)) {
    if (side->lastmode == mode) {
      printf("\t(%s not reprimed)\n", side->label);
      return 0;
    } else {
      printf("\t(repriming active %s!!)\n", side->label);
    }
  }
  return react_prime_sock(side->ready, side->sock, side->lastmode = mode);
}

static int ensure_waiting(struct conn *conn, struct side *side,
                          struct side *other,
                          react_proc_t *recvpr, react_proc_t *sendpr)
{
  printf("Ensuring that %s is being checked.\n", side->label);
  if (!react_istriggered(side->ready)) {
    if (!side->can_write) {
      printf("    Waiting to write %lu bytes to %s.\n",
             (unsigned long) (other->cap - other->mark), side->label);
      return prime(conn, react_MOUT, sendpr, side);
    } else if (!side->terminated && side->cap < BUFFER_SIZE) {
      /* We are still expecting data from 'side', and we have space
         for it. */
      printf("    Waiting to read upto %lu from %s.\n",
             (unsigned long) (BUFFER_SIZE - side->cap), side->label);
      return prime(conn, react_MIN, recvpr, side);
    }
  } else
    printf("    Already waiting.\n");

  return 0;
}

static void on_send_upstream(void *vc);
static void on_send_downstream(void *vc);
static void on_recv_upstream(void *vc);
static void on_recv_downstream(void *vc);

static void ensure_all(struct conn *conn, int flag)
{
  if (flag)
    ensure_waiting(conn, &conn->upstream, &conn->downstream,
                   &on_recv_upstream, &on_send_upstream);
  ensure_waiting(conn, &conn->downstream, &conn->upstream,
                 &on_recv_downstream, &on_send_downstream);
}

static void on_send_upstream(void *vc)
{
  struct conn *conn = vc;
  ensure_all(conn, handle_send(conn, &conn->downstream, &conn->upstream));
}

static void on_recv_upstream(void *vc)
{
  struct conn *conn = vc;
  int flag = handle_recv(conn, &conn->upstream, &conn->downstream);
  if (conn->downstream.terminated ||
      (conn->upstream.terminated &&
       conn->upstream.cap == conn->upstream.mark)) {
    close_conn(conn);
    return;
  }
  ensure_all(conn, flag);
}

static void on_send_downstream(void *vc)
{
  struct conn *conn = vc;
  ensure_all(conn, handle_send(conn, &conn->upstream, &conn->downstream));
}

static void on_recv_downstream(void *vc)
{
  struct conn *conn = vc;
  int flag = handle_recv(conn, &conn->downstream, &conn->upstream);
  if (conn->downstream.terminated ||
      (conn->upstream.terminated &&
       conn->upstream.cap == conn->upstream.mark)) {
    close_conn(conn);
    return;
  }
  ensure_all(conn, flag);
}

static void on_connected(void *vc);

static int prime_connected(struct conn *conn)
{
  return prime(conn, react_MOUT, &on_connected, &conn->upstream);
}

static void on_connected(void *vc)
{
  if (handle_connected(vc))
    ensure_all(vc, 1);
  else
    close_conn(vc);
}

static struct conn *open_conn(struct proxy *proxy,
                              const struct sockaddr_in *from,
                              react_sock_t sock)
{
  react_ioctlflag_t flag = 1;
  struct conn *conn = malloc(sizeof *conn);
  if (!conn) {
    closesocket(sock);
    goto error;
  }
  conn->upstream.ready = conn->downstream.ready = react_ERROR;
  conn->downstream.sock = sock;
  conn->upstream.sock = react_INVALID_SOCKET;
  conn->proxy = NULL;

  printf("Socket created is %03d\n", (int) conn->downstream.sock);

  /* Create bindings for traffic from the client and server. */
  conn->upstream.ready = react_open(proxy->core);
  if (conn->upstream.ready == react_ERROR)
    goto error;
  conn->downstream.ready = react_open(proxy->core);
  if (conn->downstream.ready == react_ERROR)
    goto error;
  conn->upstream.cap = conn->upstream.mark = 0;
  conn->downstream.cap = conn->downstream.mark = 0;
  conn->upstream.can_write = conn->downstream.can_write = 0;
  conn->upstream.terminated = conn->downstream.terminated = 0;
  conn->upstream.nonblocking = conn->downstream.nonblocking = 0;
  conn->upstream.lastmode = conn->downstream.lastmode = react_M_MAX;
  strcpy(conn->upstream.label, "upstream");
  strcpy(conn->downstream.label, "downstream");

  printf("Creating upstream socket...\n");

  /* Create a non-blocking socket to reach the server. */
  conn->upstream.sock = socket(PF_INET, SOCK_STREAM, 0);
  if (conn->upstream.sock == react_INVALID_SOCKET)
    goto error;
  printf("Socket created is %03d\n", (int) conn->upstream.sock);

  printf("Making non-blocking...\n");

  /* Make it non-blocking. */
  if (ioctlsocket(conn->upstream.sock, FIONBIO, &flag) == react_SOCKET_ERROR)
    goto error;
  conn->upstream.nonblocking = 1;

  printf("Performing non-blocking connect to %s:%d...\n",
         inet_ntoa(proxy->target.sin_addr), ntohs(proxy->target.sin_port));

  /* Set off the connection to the main server, and get ready to
     respond to it. */
  if (connect(conn->upstream.sock, (struct sockaddr *) &proxy->target,
              sizeof proxy->target) == react_SOCKET_ERROR) {
#ifdef __WIN32__
    if (WSAGetLastError() != WSAEWOULDBLOCK)
      goto error;
#else
    if (errno != EINPROGRESS) {
      perror("connect");
      goto error;
    }
#endif
  }

  if (prime_connected(conn) < 0)
    goto error;

  /* Add the connection to the proxy. */
  conn->proxy = proxy;
  dllist_append(&conn->proxy->conns, others, conn);

  return conn;

 error:
  if (conn) {
    if (conn->upstream.ready != react_ERROR)
      react_close(conn->upstream.ready);
    if (conn->downstream.ready != react_ERROR)
      react_close(conn->downstream.ready);
    if (conn->downstream.sock != react_INVALID_SOCKET)
      closesocket(conn->downstream.sock);
    if (conn->upstream.sock != react_INVALID_SOCKET)
      closesocket(conn->upstream.sock);
    if (conn->proxy)
      dllist_unlink(&conn->proxy->conns, others, conn);
    free(conn);
  }
  return NULL;
}

static int handle_accepting(struct proxy *proxy)
{
  struct sockaddr_in from;
  socklen_t fromlen = sizeof from;

  react_sock_t sock =
    accept(proxy->server, (struct sockaddr *) &from, &fromlen);
  if (sock == react_INVALID_SOCKET) {
    perror("accept");
  } else {
    /* Create a new connection for this socket. */
    open_conn(proxy, &from, sock);
  }

  return 1;
}

static int prime_accepting(struct proxy *proxy)
{
  return react_prime_sock(proxy->accepting, proxy->server, react_MIN);
}

static void on_accepting(void *vp)
{
  if (handle_accepting(vp))
    prime_accepting(vp);
}

int init_proxy(struct proxy *proxy, react_core_t core,
               const struct sockaddr_in *target,
               const struct sockaddr_in *local)
{
  /* Set everything to a safe state. */
  proxy->accepting = react_ERROR;
  proxy->server = react_INVALID_SOCKET;
  dllist_init(&proxy->conns);
  proxy->core = core;
  proxy->target = *target;

  /* Try to open a reactor binding. */
  proxy->accepting = react_open(core);
  if (proxy->accepting == react_ERROR)
    goto error;
  react_direct(proxy->accepting, &on_accepting, proxy);

  /* Try to open a socket. */
  proxy->server = socket(PF_INET, SOCK_STREAM, 0);
  if (proxy->server == react_INVALID_SOCKET)
    goto error;
  printf("Socket created is %03d\n", (int) proxy->server);

  printf("Binding server socket to %s:%d...\n",
         inet_ntoa(local->sin_addr), ntohs(local->sin_port));

  if (bind(proxy->server, (const struct sockaddr *) local,
           sizeof *local) == react_SOCKET_ERROR)
    goto error;

  printf("Setting back-log size...\n");

  if (listen(proxy->server, 5) == react_SOCKET_ERROR)
    goto error;

  printf("Preparing for accept()...\n");

  /* Register to wait on the socket. */
  if (prime_accepting(proxy) < 0)
    goto error;

  printf("ready to go\n");

  return 0;

 error:
  if (proxy->accepting != react_ERROR)
    react_close(proxy->accepting);
  if (proxy->server != react_INVALID_SOCKET)
    closesocket(proxy->server);
     
  return -1;
}

static int getaddr(struct sockaddr_in *addr, const char *str)
{
  const char *colon = strchr(str, ':');
  memset(addr, 0, sizeof *addr);
  addr->sin_family = AF_INET;
  if (colon) {
    char temp[80];
    struct hostent *hp;
    if (snprintf(temp, sizeof temp, "%.*s", (int) (colon - str), str) < 0)
      return -1;
    hp = gethostbyname(temp);
    if (!hp)
      return -1;
    memcpy(&addr->sin_addr, hp->h_addr, 4);
    str = colon + 1;
  } else {
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
  }
  addr->sin_port = htons(atoi(str));
  return 0;
}

int main(int argc, const char *const *argv)
{
#ifdef __WIN32__
  WSADATA wsaData;
  {
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0) {
      perror("WSAStartup");
      return EXIT_FAILURE;
    }
  }
#endif

  int rc = EXIT_SUCCESS;
  react_core_t core;
  struct sockaddr_in local, remote;

  if (argc < 3) {
    fprintf(stderr, "usage: %s local-addr remote-addr\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (getaddr(&local, argv[1]) < 0) {
    fprintf(stderr, "%s: couldn't resolve %s\n", argv[0], argv[1]);
    return EXIT_FAILURE;
  }

  if (getaddr(&remote, argv[2]) < 0) {
    fprintf(stderr, "%s: couldn't resolve %s\n", argv[0], argv[2]);
    return EXIT_FAILURE;
  }

#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif

  core = react_opencore(3);
  if (core != react_COREERROR) {
    struct proxy proxy;
    if (init_proxy(&proxy, core, &remote, &local) < 0) {
      perror("init_proxy");
      rc = EXIT_FAILURE;
    } else {
      printf("Entering polling loop...\n");
      for ( ; ; ) {
        if (react_yield(core) < 0) {
          perror("react_yield");
          rc = EXIT_FAILURE;
          break;
        }
      }
      printf("Out of it now %d.\n", errno);
    }
    react_closecore(core);
  } else {
    perror("react_opencore");
    rc = EXIT_FAILURE;
  }

  return rc;
}
