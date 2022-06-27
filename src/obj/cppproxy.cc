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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <system_error>

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

#include <ddslib/dllist.hh>

#include "react/core.hh"
#include "react/event.hh"
#include "react/handlerhelp.hh"
#include "react/idle.hh"
#include "react/socket.hh"

#ifndef __WIN32__
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#ifdef __WIN32__
typedef u_long ioctlflag_t;
#else
typedef int ioctlflag_t;
typedef int SOCKET;
#endif

#if defined __WIN32__
#elif defined __riscos || defined __riscos__
#define closesocket(X) close(X)
#define ioctlsocket(X,Y,Z) ioctl(X,Y,Z)
#else
#define closesocket(X) close(X)
#define ioctlsocket(X,Y,Z) ioctl(X,Y,Z)
#endif

#define CONN_PRIO 2
#define ACCEPT_PRIO 1
#define CLEAN_PRIO 0

using namespace std;

class Connection;

class Stream {
  bool more, complete;

  const struct sockaddr_in *remote;

  SOCKET source, dest;
  char buf[256];
  size_t start, end;

  Connection &holder;
  void (Connection::*whenDone)();

  react::InnerHandler<Stream> writeDelegate, readDelegate;
  react::Event writeEvent, readEvent;

  void ensureRead(), ensureWrite();
  void readyToWrite();
  void readyToRead();

  void checkTermination();

public:
  Stream(react::Core core, react::prio_t prio,
         SOCKET from, SOCKET to,
         Connection &holder, void (Connection::*whenDone)(),
         const struct sockaddr_in *remote);
  ~Stream();

  void go() {
    ensureRead();
    ensureWrite();
  }
};

class Proxy;

class Connection {
  react::Core core;
  react::prio_t prio;
  SOCKET server, client;
  Stream *upstream, *downstream;
  bool moreUpstream, moreDownstream;
  struct sockaddr_in remote;
  Proxy &holder;
  void (Proxy::*whenDone)(Connection *);

  ddslib::dllist::element<Connection> others;

  void checkEnd();
  void upstreamDone();
  void downstreamDone();

public:
  Connection(react::Core core,
             react::prio_t prio,
             SOCKET client,
             const struct sockaddr_in &remote,
             Proxy &holder,
             void (Proxy::*whenDone)(Connection *));
  ~Connection();

  void go();

  friend class Proxy;
};

class Proxy {
  react::Core core;
  struct sockaddr_in remote;
  SOCKET server;

  ddslib::dllist::header<Connection> terminated;

  react::InnerHandler<Proxy> acceptDelegate, cleanDelegate;
  react::Event acceptEvent, cleanEvent;

  void acceptReady(), clean();

  void finished(Connection *);

  void ensureAccept();

public:
  Proxy(react::Core core,
        const struct sockaddr_in &local,
        const struct sockaddr_in &remote);
  ~Proxy();

  void go() { ensureAccept(); }
};

static int getaddr(struct sockaddr_in &addr, const char *str)
{
  const char *colon = strchr(str, ':');
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  if (colon) {
    char temp[80];
    struct hostent *hp;
    if (snprintf(temp, sizeof temp, "%.*s", (int) (colon - str), str) < 0)
      return -1;
    hp = gethostbyname(temp);
    if (!hp)
      return -1;
    memcpy(&addr.sin_addr, hp->h_addr, 4);
    str = colon + 1;
  } else {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  addr.sin_port = htons(atoi(str));
  return 0;
}

[[noreturn]] static inline void throw_errno() {
  throw system_error(errno, system_category());
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

  struct sockaddr_in local, remote;

  if (argc < 3) {
    cerr << "usage: " << argv[0] << " local-addr remote-addr" << endl;
    return EXIT_FAILURE;
  }

  if (getaddr(local, argv[1]) < 0) {
    cerr << argv[0] << ": couldn't resolve " << argv[1] << endl;
    return EXIT_FAILURE;
  }

  if (getaddr(remote, argv[2]) < 0) {
    cerr << argv[0] << ": couldn't resolve " << argv[2] << endl;
    return EXIT_FAILURE;
  }

  react::Core core = react::newCore(3);
  if (core) {
    Proxy proxy(core, local, remote);
    proxy.go();

    for ( ; ; ) {
      try {
        core->yield();
      } catch (int en) {
        cerr << strerror(en) << endl;
      }
    }
  }

  return EXIT_SUCCESS;
}



void Stream::checkTermination()
{
  if (complete)
    return;

  if (!more && start == end) {
    complete = true;
    (holder.*whenDone)();
  }
}

void Stream::readyToWrite()
{
  if (remote) {
    cout << "Completing connection to " << inet_ntoa(remote->sin_addr)
         << ":" << ntohs(remote->sin_port) << endl;

    if (connect(dest, (struct sockaddr *) remote, sizeof *remote) ==
        SOCKET_ERROR) {
      cerr << strerror(errno) << endl;
      throw_errno();
    }

    remote = 0;

    ensureWrite();
    return;
  }

  int done = send(dest, buf + start, end - start, 0);
  if (done < 0) {
    // erm
  } else {
    start += done;

    if (end - start < 16) {
      memmove(buf, buf + start, end - start);
      end -= start;
      start = 0;
    }

    ensureRead();
    ensureWrite();

    checkTermination();
  }
}

void Stream::readyToRead()
{
  int got = recv(source, buf + end, sizeof buf - end, 0);
  if (got == 0) {
    more = false;
    checkTermination();
  } else if (got < 0) {
    // erm
  } else {
    end += got;

    ensureRead();
    ensureWrite();
  }
}

void Stream::ensureRead()
{
  if (end == sizeof buf) return;
  if (readEvent->primed()) return;
  react::SocketCondition cond(source, react_MIN);
  readEvent->prime(cond);
}

void Stream::ensureWrite()
{
  if (start == end && !remote) return;
  if (writeEvent->primed()) return;
  react::SocketCondition cond(dest, react_MOUT);
  writeEvent->prime(cond);
}

Stream::Stream(react::Core core, react::prio_t prio,
               SOCKET from, SOCKET to,
               Connection &holder, void (Connection::*whenDone)(),
               const struct sockaddr_in *remote)
  : more(true), complete(false),
    remote(remote), source(from), dest(to), start(0), end(0),
    holder(holder), whenDone(whenDone),
    writeDelegate(*this, &Stream::readyToWrite),
    readDelegate(*this, &Stream::readyToRead),
    writeEvent(core->open(writeDelegate, prio)),
    readEvent(core->open(readDelegate, prio)) { }

Stream::~Stream()
{
}


void Connection::checkEnd()
{
  if (moreDownstream) return;

  (holder.*whenDone)(this);
}

void Connection::upstreamDone()
{
  cout << "No more upstream" << endl;
  moreUpstream = false;
  checkEnd();
}

void Connection::downstreamDone()
{
  cout << "No more downstream" << endl;
  moreDownstream = false;
  checkEnd();
}

Connection::Connection(react::Core core,
                       react::prio_t prio,
                       SOCKET client,
                       const struct sockaddr_in &remote,
                       Proxy &holder,
                       void (Proxy::*whenDone)(Connection *))
  : core(core), prio(prio),
    server(SOCKET_ERROR), client(client),
    upstream(0), downstream(0),
    moreUpstream(true), moreDownstream(true),
    remote(remote),
    holder(holder), whenDone(whenDone) { }

void Connection::go()
{
  // Open the socket.
  server = socket(PF_INET, SOCK_STREAM, 0);
  if (server == SOCKET_ERROR)
    throw_errno();

  ioctlflag_t flag = 1;
  if (ioctlsocket(server, FIONBIO, &flag) == SOCKET_ERROR)
    throw_errno();

  cout << "Initiating connection to " << inet_ntoa(remote.sin_addr)
       << ":" << ntohs(remote.sin_port) << endl;

  if (connect(server, (struct sockaddr *) &remote, sizeof remote) ==
      SOCKET_ERROR) {
#ifdef __WIN32__
    if (WSAGetLastError() != WSAEWOULDBLOCK)
      throw_errno();
#else
    if (errno != EINPROGRESS)
      throw_errno();
#endif
  }

  cout << "Starting streams..." << endl;

  upstream = new Stream(core, prio, client, server,
                        *this, &Connection::upstreamDone, &remote);
  downstream = new Stream(core, prio, server, client,
                          *this, &Connection::downstreamDone, 0);

  upstream->go();
  downstream->go();

  //cerr << "Here " << __FILE__ << ", line " << __LINE__ << endl;
}

Connection::~Connection()
{
  if (upstream)
    delete upstream;
  if (downstream)
    delete downstream;
  if (server != SOCKET_ERROR)
    closesocket(server);
  if (client != SOCKET_ERROR)
    closesocket(client);
}

void Proxy::acceptReady()
{
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof addr;

  int client = accept(server, (struct sockaddr *) &addr, &addrlen);
  if (client == SOCKET_ERROR)
    throw_errno();

  cout << "Accepted " << inet_ntoa(addr.sin_addr)
       << ":" << ntohs(addr.sin_port) << endl;
  Connection *conn = new Connection(core, CONN_PRIO, 
                                    client, remote,
                                    *this, &Proxy::finished);
  conn->go();

  ensureAccept();
}

void Proxy::clean()
{
  while (!terminated.is_empty()) {
    Connection *cp = terminated.first();
    terminated.unlink(&Connection::others, cp);
    delete cp;
  }
}

void Proxy::finished(Connection *cp)
{
  terminated.append(&Connection::others, cp);
  if (cleanEvent->primed()) return;
  react::IdleCondition cond;
  cleanEvent->prime(cond);
}

Proxy::Proxy(react::Core core,
             const struct sockaddr_in &local,
             const struct sockaddr_in &remote)
  : core(core), remote(remote), server(SOCKET_ERROR),
    acceptDelegate(*this, &Proxy::acceptReady),
    cleanDelegate(*this, &Proxy::clean),
    acceptEvent(core->open(acceptDelegate, ACCEPT_PRIO)),
    cleanEvent(core->open(cleanDelegate, CLEAN_PRIO))
{
  server = socket(PF_INET, SOCK_STREAM, 0);
  if (server == SOCKET_ERROR)
    throw_errno();

  if (bind(server, (struct sockaddr *) &local, sizeof local) == SOCKET_ERROR)
    throw_errno();

  socklen_t locallen = sizeof local;
  if (getsockname(server,
                  (struct sockaddr *) &local, &locallen) == SOCKET_ERROR)
    throw_errno();

  if (listen(server, 5) == SOCKET_ERROR)
    throw_errno();

  cout << "New proxy at " << inet_ntoa(local.sin_addr)
       << ":" << ntohs(local.sin_port) << endl;
}

Proxy::~Proxy()
{
  closesocket(server);
}

void Proxy::ensureAccept()
{
  if (acceptEvent->primed()) return;

  react::Condition_SOCK cond(server, react_MIN);
  acceptEvent->prime(cond);
}
