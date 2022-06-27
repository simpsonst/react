# Purpose

This library is intended to support applications designed to operate in an event-driven way, usually with a single thread.
The reactor pattern allows the application to register an arbitrary and dynamic set of events which the application wishes to react to, and associate actions to be carried out under those events.
A reactor 'core' maintains pending events, and works out which system calls should be made to allow the application to yield processing time to the system when no behaviour is required.

Furthermore, different libraries that compose the application can be made to interoperate smoothly, in that their event sets are transparently merged, and can present common APIs to each other.
This allows very modular development of portable applications.

# Installation

The `Makefile` attempts to read local configuration from a file called `config.mk` adjacent to it, then from `react-env.mk` on the include path, so if you want to override the default macros, put your own definitions in one of those.

As a matter of course, you might want to create a directory, e.g., `~/.local/include`, set `MAKEFLAGS` to contain `-I$HOME/.local/include`, and put such local configurations in there.
That way, you can remove and re-unpack this package as often as you like, without losing the local configuration.


## Dependencies and build options

[Binodeps](https://github.com/simpsonst/binodeps) is required to use the `Makefile`.

[DDSLib](https://github.com/simpsonst/ddslib) is required to compile.

Your `react-env.mk` or `config.mk` should include:

```
CFLAGS += -D_POSIX_C_SOURCE=199309L
```

&hellip;or:

```
CFLAGS += -D_POSIX_SOURCE=1
```

&hellip;particularly if compiling with strict language settings.

Actually, `-std=c99 -D_XOPEN_SOURCE=600` (or similar) is often sufficient on POSIX systems.

Try:

```
CFLAGS += -std=gnu11 
CXXFLAGS += -std=gnu++11
```

To cross-compile to Windows, use `c11` and `c++11`, as some of the GNU extensions will confuse the feature testing.


## Targets

`make` or `make all` builds the library.

`make install` installs these files in $(PREFIX):

```
lib/libreact.a
include/react/features.h
include/react/types.h
include/react/version.h
include/react/prio.h
include/react/core.h
include/react/event.h
include/react/conds.h
include/react/condtype.h
include/react/user.h
include/react/idle.h
include/react/socket.h
include/react/time.h
include/react/fd.h
include/react/pipe.h
include/react/file.h
include/react/riscos.h
include/react/windows.h
include/react/signal.h
```

If `ENABLE_CXX` is not set to anything but `yes`, these are also installed:

```
lib/libreact++.a
include/react/condition.hh
include/react/handler.hh
include/react/prio.hh
include/react/version.hh
include/react/core.hh
include/react/event.hh
include/react/condhelp.hh
include/react/conds.hh
include/react/handlerhelp.hh
include/react/user.hh
include/react/idle.hh
include/react/time.hh
include/react/windows.hh
include/react/socket.hh
include/react/fd.hh
include/react/types.hh
include/react/file.hh
include/react/pipe.hh
```


`make out/testreact` builds a test program.
It schedules three tickers at 0.5Hz, 1Hz and 2Hz.
It also echos lines typed at the terminal, and runs an idle event printing a bucket of stars.
The tickers each add varying numbers of stars to the bucket, and expire after varying numbers of ticks.
Each line typed at the terminal adds the same number of stars as characters typed.

`make out/proxy` builds a test proxy (well, a tunnel really) which should compile on Windows and Linux.
To run the program `proxy`, give it two arguments:
a local address to bind to, and a remote address to connect to.
Addresses take the form `[hostname:]portnum`.
Anything connecting to the local address will be tunnelled straight to the remote address.

`make out/cppproxy out/cpptest` builds C++ versions of these programs.


# Use

Here are some examples of use.
You will normally create one core object per program, though you might have one per thread in a multithreaded program:

```
#include <stdio.h>
#include <errno.h>
#include <react/core.h>

react_core_t core;
struct proxy prox;
struct audio aud;

// Create a core.
core = react_opencore(0);
if (core == react_COREERROR) {
  perror("react_opencore");
  exit(EXIT_FAILURE);
}

/* Create the portable objects of your program, telling then
   about the core. */
init_proxy(&prox, core, /* other config args */);
init_audio(&aud, core, /* other config args */);

// Process events.
while (more_work) {
  if (react_yield(core) < 0) {
    if (errno == EAGAIN)
      break;
    perror("react_yield");
    break;
  }
}

// Tidy up at end of program.
react_closecore(core);
```

For each pending event, you need a distinct handle of type `react_t`:

```
#include <stdio.h>
#include <react/event.h>
#include <react/idle.h>

react_t ev = react_open(core);
if (ev == react_ERROR) {
  perror("react_open");
  return -1;
}

// Call my_func(NULL) (the handler) when the event occurs.
void my_func(void *);
react_direct(ev, &my_func, NULL);

// Invoke the handler the next time the core is idle.
if (react_prime_idle(ev) < 0) {
  perror("react_prime(idle)");
  return -1;
}
```

You can re-use event handles as much as you like, e.g., cancel, reprime, whether primed or not.
The typical event types are:

```
react_t ev = ...;

// when the program is idle
#include <react/idle.h>
react_prime_idle(ev);

// at the given time
#include <sys/time.h>
#include <react/time.h>
struct timeval tv = ...;
react_prime_timeval(ev, &tv);

// when the socket is readable
#include <react/fd.h>
react_prime_fd(ev, sock, react_MIN); // or MOUT, MEXC
```

## C interface

The primary types are `react_core_t`, defined in `<react/core.h>`, representing a reactor/core, and `react_t`, defined in `<react/event.h>`, representing an event handle.
Each event handle belongs to one reactor during its lifetime.
It has a priority (highest by default), and can be unprimed (the default) or primed, unqueued (the default) or queued.
When control is yielded to a reactor, its event handles can be queued if the events they are primed with occur.
Queued handles are later processed by priority, which then makes them unqueued and unprimed.

An event handle must be set to invoke a user-defined function of type `void ()(void *)` when processed.
Priming a handle associates it with a particular event, such as a file descriptor becoming readable.
If the event occurs, and the core detects it, the handle will be dissociated from the event and queued, and eventually the user-defined function will be invoked.

Cancelling an event handle also dissociates it from any event it is primed against, and returns it to the unprimed state.
If it is already queued but not processed, it will not invoke the user-defined function before returning to the unprimed state.

Destroying an event handle also cancels it first.
Destroying a reactor dissociates all its event handles from itself, but doesn't destroy them.

```
#include <react/core.h>

react_core_t react_opencore(size_t prios);
void react_closecore(react_core_t);

int react_yield(react_core_t core);
```

`react_opencore` creates a new reactor and returns it.
It will support `prios` distinct priorities.
The function returns `react_COREERROR` on failure, and might set `errno`.

`react_closecore` destroys a reactor.

```
// Only on systems with sigset_t
#include <signal.h>
#include <react/core.h>
#if react_HAVE_SIGSET
sigset_t *react_sigmask(react_core_t core);
#endif
```

You can specify the set of signals to remain blocked when yielding to the reactor.
The core retains a `sigset_t` (default empty), and `react_sigmask` allows you to access it.
Implementations that use `ppoll` or `pselect` will pass this set to that call, atomically and temporarily enabling signals in the complement of the set.
Implementations that use `poll` or `select` will emulate this behaviour less robustly.

## Event-handle management

```
#include <react/core.h>
#include <react/event.h>

react_t react_open(react_core_t core);
int react_close(react_t ev);
```

`react_open` creates an event handle for the specified reactor, or returns `react_ERROR` on failure, setting `errno` to `react_ENOMEM`.

`react_close` destroys an event handle.

## Handle configuration

```
#include <react/event.h>

typedef void react_proc_t(void *);
void react_direct(react_t ev, react_proc_t *func, void *ctxt);

void react_getprios(react_t ev, react_prio_t *p, react_subprio_t *sp);
int react_setprios(react_t ev, react_prio_t p, react_subprio_t sp);
```

`react_direct` makes the event handle invoke `(*func)(ctxt)` when it is triggered.

`react_setprios` sets the major and minor priorities of the event handle `ev` to `p` and `sp`.
It returns non-zero on failure, setting `errno` to `react_EBADSTATE` if the event handle is queued.
`react_getprios` sets `*p` and `*sp` to the event handle's current major and minor priorities.


## Priming

The following functions prime an event handle on various types of events.
If the event handle is already primed or queued, it is cancelled first.

Some are not available on some platforms.
The headers will exist, but the functions will not be declared, and they will not link.

### Idle events

```
#include <react/idle.h>
int react_prime_idle(react_t ev);
```

The handle will be queued on the next call to `react_yield`.
Use it with a low major priority to do work when otherwise idle.

### Timed events

```
#include <react/time.h>
int react_prime_timeval(react_t, const struct timeval *tp);
int react_prime_timespec(react_t, const struct timespec *tp);
```

```
#include <windows.h>
#include <react/time.h>
int react_prime_winfiletime(react_t, const FILETIME *tp);
```

```
#include <time.h>
#include <react/time.h>
int react_prime_stdtime(react_t, const time_t *tp);
```

The handle will be queued at the time specified by `*tp`, or soon after.

### File-descriptor events

```
#include <react/fd.h>
int react_prime_fd(react_t, int fd, react_iomode_t m);
int react_prime_fdin(react_t, int fd);
int react_prime_fdout(react_t, int fd);
int react_prime_fdexc(react_t, int fd);
```

When `m` is `react_MIN`, `react_MOUT` or `react_MEXC`, the handle will be queued when descriptor `fd` can be read from, written to, or accessed for exceptional events, without blocking.
The other functions simply use a hard-wired mode as indicated by their names.

```
#include <react/fd.h>
int react_prime_read(react_t, int fd, void *buf, size_t len,
                     ssize_t *rc, int *en);
```

The handle will be queued when [1, `len`] bytes have been read from `fd` into the array at `buf`.
The number of bytes read is stored in `*rc`.
If EOF was detected, `*rc` will be zero.
On error, `*rc` will be `-1`, and `*en` will contain the error code.

```
#include <react/fd.h>
int react_prime_write(react_t, int fd, const void *buf, size_t len,
                      ssize_t *rc, int *en);
```

The handle will be queued when [1, `len`] bytes have been written to `fd` from the array at `buf`.
The number of bytes written is stored in `*rc`. On error, `*rc` will be `-1`, and `*en` will contain the error code.

```
#include <sys/uio.h>
#include <react/fd.h>
int react_prime_readv(react_t, int fd, const struct iovec *iov,
                      int niovs, ssize_t *rc, int *en);
```

The handle will be queued when at least one byte has been read from `fd` into the buffers specified by `iov[0]`&hellip;`iov[niovs-1]`.
The number of bytes read is stored in `*rc`.
If EOF was detected, `*rc` will be zero.
On error, `*rc` will be `-1`, and `*en` will contain the error code.

```
#include <sys/uio.h>
#include <react/fd.h>
int react_prime_writev(react_t, int fd, const struct iovec *iov,
                       int niovs, ssize_t *rc, int *en);
```

The handle will be queued when at least one byte from the buffers specified by `iov[0]`&hellip;`iov[niovs-1]` has been written to `fd`.
The number of bytes written is stored in `*rc`.
On error, `*rc` will be `-1`, and `*en` will contain the error code.

```
#include <sys/select.h>
#include <react/fd.h>
int react_prime_fds(react_t, int nfds,
                    fd_set *in, fd_set *out, fd_set *exc);
```

The handle will be queued when at least one of the descriptors in `*in`, `*out` and `*exc` can be accessed without blocking.
Only descriptors `0`&hellip;`nfds-1` are considered in each set.
Any of `in`, `out` and `exc` may be `NULL` to indicate an empty set, but at least one descriptor must be specified.
If, due to priority, the handle is not processed during the call to `react_yield` that queued it, it will continue to watch for remaining events that have not yet happened.
When processed, `*in`, `*out` and `*exc` will contain all events that were detected since it was primed.

```
#include <poll.h>
#include <react/fd.h>
int react_prime_poll(react_t, int fd, short evs, short *got);
int react_prime_polls(react_t, struct pollfd *arr, nfds_t len);
```

With `react_prime_poll`, the handle is queued when any of the events in `evs` occur on descriptor `fd`.
If `got` is not `NULL`, the number of actual events that have occurred are written to `*got`.
The possible events are `POLLIN`, `POLLOUT`, etc, as specified for `poll()`.
`POLLHUP`, `POLLERR` and `POLLNVAL` are always implicitly watched for.

With `react_prime_polls`, the handle is queued if any of the events specified in the array `arr[0]`&hellip;`arr[len-1]` occur.
The `fd` field of each element may be negative to be ignored, but at least one descriptor must be specified.
The events that have occurred on each descriptor are provided in the corresponding field revents.

With both of these functions, if the handle is not processed as soon as it is queued due to priority, it will continue to monitor for remaining events, and accumulate them as they happen.

```
#include <fcntl.h>
#include <react/fd.h>
int react_prime_splice(react_t,
                       int fd_in, loff_t *off_in,
                       int fd_out, loff_t *off_out, size_t max,
                       unsigned flags, ssize_t *rc, int *en);
int react_prime_splice_writefirst(react_t,
                                  int fd_in, loff_t *off_in,
                                  int fd_out, loff_t *off_out, size_t max,
                                  unsigned flags, ssize_t *rc, int *en);
int react_prime_splice_dual(react_t,
                            int fd_in, loff_t *off_in,
                            int fd_out, loff_t *off_out, size_t max,
                            unsigned flags, ssize_t *rc, int *en);
```

The handle will be queued when `fd_out` has become writable, and `fd_in` becomes readable, and upto `max` bytes have been moved from `fd_in` to `fd_out`.
The number moved is written to `*rc`, or negative on error, with `*en` containing the error code.
`*off_in` must be an offset to read from, or `off_in` must be `NULL`.
`*off_out` must be an offset to write to, or `off_out` must be `NULL`.
`*off_in` and `*off_out` will be updated by the number of bytes moved.

`react_prime_slice_dual` uses `react_prime_polls` (or `react_prime_fds` if `poll` is not available) internally to monitor both descriptors simultaneously.
If only one event occurs, it remembers it, and waits only for the remainder.
`react_prime_slice` is an alias macro for `react_prime_slice_dual`.

`react_prime_slice_writefirst` first waits for the output descriptor to become writable, then for the input to become readable, so it always takes two events.
It is retained as the original implementation.

`*rc` takes on the following error codes:

- `-1` &ndash; The error was generated by the internal call to splice.
- `-2` &ndash; The error was due to internal repriming of the event on the input descriptor.
- `-3` &ndash; The error was due to internal repriming of the event on the output descriptor.
- `-4` &ndash; An unexpected event ocurred on the input descriptor. Note that en error might also have occurred on the output descriptor.
- `-5` &ndash; An unexpected event ocurred on the output descriptor. Note that en error might also have occurred on the input descriptor.

### Socket events

```
#ifdef __WIN32__
#include <winsock2.h>
#endif
#include <react/socket.h>
int react_prime_sock(react_t, react_sock_t sock, react_iomode_t m);
int react_prime_sockin(react_t, react_sock_t sock);
int react_prime_sockout(react_t, react_sock_t sock);
int react_prime_sockexc(react_t, react_sock_t sock);
```

When `m` is `react_MIN`, `react_MOUT` or `react_MEXC`, the handle will be queued when socket `sock` can be read from, written to, or accessed for exceptional events, without blocking.
The other functions simply use a hard-wired mode as indicated by their names.

`react_sock_t` is `SOCKET` on Windows, but `int` everywhere else.
These functions are provided for portability of programs that use sockets.

```
#include <sys/types.h>
#include <sys/socket.h>
#include <react/socket.h>
int react_prime_recv(react_t, react_sock_t sock,
                     void *buf, react_buflen_t len, int flags,
                     react_ssize_t *rc, int *en);
int react_prime_recvfrom(react_t, react_sock_t sock,
                         void *buf, react_buflen_t len, int flags,
                         struct sockaddr *addr,
                         react_socklen_t *addrlen,
                         react_ssize_t *rc, int *en);
```

The handle is queued and defused when at least one byte has been read from `sock` into `buf`.
At most `len` bytes are read.
The number of bytes read is stored in `*rc`, or `-1` on error, with the error code stored in `*en`.
Use `react_prime_recv` on a stream socket, and `react_prime_recvfrom` on a datagram socket, the latter storing the address of the sender in `*addr`, and specifying the address length in `*addrlen`.

```
#include <sys/types.h>
#include <sys/socket.h>
#include <react/socket.h>
int react_prime_send(react_t, react_sock_t sock,
                     const void *buf, react_buflen_t len, int flags,
                     react_ssize_t *rc, int *en);
int react_prime_sendto(react_t, react_sock_t sock,
                       const void *buf, react_buflen_t, int flags,
                       const struct sockaddr *addr,
                       react_socklen_t addrlen,
                       react_ssize_t *rc, int *en);
```

The handle is defused and queued when at least one byte has been written to `sock` from `buf`.
At most `len` bytes are written.
The number of bytes written is stored in `*rc`, or `-1` on error, with the error code stored in `*en`.
Use `react_prime_send` on a stream socket, and `react_prime_sendto` on a datagram socket, for the latter giving the address of the receiver in `*addr`, and specifying the address length in `addrlen`.

```
#include <sys/types.h>
#include <sys/socket.h>
#include <react/socket.h>
int react_prime_recvmsg(react_t, react_sock_t sock,
                        struct msghdr *msg, int flags,
                        react_ssize_t *rc, int *en);
```

The handle is defused and queued when some bytes specified by `msg` (see `recvmsg`) have been read from `sock`.
`*rc` holds the number of bytes received, or zero on EOF, or `-1` on error, with `*en` holding the error code.

```
#include <sys/types.h>
#include <sys/socket.h>
#include <react/socket.h>
int react_prime_sendmsg(react_t, react_sock_t sock,
                        const struct msghdr *msg, int flags,
                        react_ssize_t *rc, int *en);
```

The handle is defused and queued when some bytes specified by `msg` (see `sendmsg`) have been written to `sock`.
`*rc` holds the number of bytes written, or `-1` on error, with `*en` holding the error code.

```
#include <sys/types.h>
#include <sys/socket.h>
#include <react/socket.h>
int react_prime_accept(react_t, react_sock_t sock,
                       struct sockaddr *addr,
                       react_socklen_t *addrlen,
                       react_sock_t *created, int *en);
int react_prime_accept4(react_t, react_sock_t sock,
                        struct sockaddr *addr,
                        react_socklen_t *addrlen, int flags,
                        react_sock_t *created, int *en);
```

The handle is defused and queued when an incoming connection is accepted on the listening socket `sock`.
`*created` will be the socket for the new connection, with the remote peer's address stored in `*addr`, and its length stored in `*addrlen`.
On error, `*created` will be `react_INVALID_SOCKET`, with the error code in `*en`.

```
#include <sys/types.h>
#include <sys/socket.h>
#include <react/socket.h>
int react_prime_connect(react_t, react_sock_t sock,
                        const struct sockaddr *addr, react_socklen_t addrlen,
                        int *rc, int *en);
```

The handle is defused and queued when an outgoing connection on the socket sock to the address `*addr` of length `addrlen` is remotely accepted.
`*rc` will be zero on success, or `-1` on error, with `*en` holding the error code.

```
#include <winsock2.h>
#include <react/socket.h>
int react_prime_winsock(react_t, SOCKET sock,
                        long m, long *got, DWORD *st);
```

The handle is queued when any of the events in m occur on socket `sock`.
The events are as specified for `WSAEventSelect`.
If `got != NULL`, `*got` will indicate which set of events occurred.
If `st != NULL`, `*st` will be `WAIT_OBJECT_0` if everything went okay, or `WAIT_ABANDONED` if something went wrong.

### Events on native file handles

```
#include <react/types.h>
#include <react/file.h>
int react_prime_file(react_t, react_file_t fd, react_iomode_t m);
int react_prime_filein(react_t, react_file_t fd);
int react_prime_fileout(react_t, react_file_t fd);
int react_prime_fileexc(react_t, react_file_t fd);
```

When `m` is `react_MIN`, `react_MOUT` or `react_MEXC`, the event handle will be queued when the file handle `fd` can be read from, written to, or accessed for exceptional events, without blocking.
The other functions simply use a hard-wired mode as indicated by their names.

`react_file_t` is `int` if defined.


### Events on ISO file handles

```
#include <stdio.h>
#include <react/file.h>
int react_prime_stdfilein(react_t, FILE *fp);
int react_prime_stdfileout(react_t, FILE *fp);
```

The handle is queued when the stream `fp` becomes readable or writeable respectively.

```
#include <react/file.h>
int react_prime_stdin(react_t);
```

The handle is queued when `stdin` becomes readable.

### Pipe events

```
#include <react/types.h>
#include <react/pipe.h>
int react_prime_pipe(react_t, react_pipe_t, react_iomode_t);
int react_prime_pipein(react_t, react_pipe_t fd);
int react_prime_pipeout(react_t, react_pipe_t fd);
int react_prime_pipeexc(react_t, react_pipe_t fd);
```

When `m` is `react_MIN`, `react_MOUT` or `react_MEXC`, the event handle will be queued when pipe descriptor `fd` can be read from, written to, or accessed for exceptional events, without blocking.
The other functions simply use a hard-wired mode as indicated by their names.

`react_pipe_t` is `int` if defined.


### Windows events

```
#include <windows.h>
#include <react/windows.h>
int react_prime_winhandle(react_t, HANDLE hdl, DWORD *st);
```

The event handle is queued when the Windows handle `hdl` is signalled.
`*st` is set to `WAIT_OBJECT_0` if okay, and `WAIT_ABANDONED` on error.

```
#include <windows.h>
#include <react/windows.h>
int react_prime_winmsg(react_t, DWORD wakeMask);
```

The handle is queued when any Windows message specified by `wakeMask` occurs.
See `MsgWaitForMultipleObjectsEx`, the parameter `dwWakeMask`, for the list of message types that can be monitored.

```
#include <windows.h>
#include <react/windows.h>
int react_prime_winapc(react_t);
```

The handle is queued when an APC arrives.
Not sure if this will work reliably if the APC arrives when not in `react_yield`!

### ISO signals

```
#include <react/signal.h>
int react_prime_intr(react_t);
```

The handle will be queued if the `react_yield call` is interrupted by at least one signal.
It is up to the user to determine which signal has occurred.
This function exists primarily to prevent `react_yield` from failing with `EAGAIN` if it runs out of all other events to watch.
A program might use this if it can run out of all other events, but can be woken up again by an external signal like `SIGHUP`.


### Miscellaneous operations

```
#include <react/event.h>
void react_cancel(react_t ev);
void react_trigger(react_t ev);
```

`react_cancel` defuses and dequeues an event handle.
`react_trigger` artificially queues and defuses an event handle.


### Handle state

```
#include <react/event.h>

// queued
int react_isqueued(react_t ev);

// primed
int react_isprimed(react_t ev);

// queued and unprimed
int react_istriggered(react_t ev);

// queued or primed
int react_isactive(react_t ev);
```

These functions determine the state of an event handle, returning non-zero if the states described above them are true.

### C++ interface

Create a core as follows:

```
#include <react/core.hh>

react::Core core = react::newCore();
```

To invoke a method `react` when an event occurs:

```
#include <react/core.hh>
#include <react/event.hh>
#include <react/handler.hh>
#include <react/time.hh>
#include <react/idle.hh>

class MyEntity : public react::Handler {
  react::Event event;

  void react() {
    ...
  }

public:
  MyEntity(react::Core core)
    : event(core->open(*this)) { }

  void doWhenIdle() {
    react::IdleCondition cond;
    event->prime(cond);
  }

  void doAfter(int secs) {
    timeval now;
    gettimeofday(&now);
    now.tv_sec += secs;
    react::TimevalCondition cond(now);
    event->prime(cond);
  }
};
```

To invoke arbitrary methods, use a delegate for each one:

```
#include <react/core.hh>
#include <react/event.hh>
#include <react/handler.hh>

class MyEntity {
  react::InnerHandler<MyEntity> delegate;
  react::Event event;

  void action() {
    ...
  }

public:
  MyEntity(react::Core core)
    : delegate(*this, &MyEntity::action),
      event(core->open(delegate)) { }

  . . .
};
```

`Core` and `Event` are `std::shared_ptr`s, so you don't have to do any special management on them.

`<react/idle.hh>` defines a condition `IdleCondition()`.

`<react/fd.hh>` defines a condition `DescriptorCondition(int, react::iomode_t)`.

`<react/time.hh>` defines conditions `TimeValCondition(const ::timeval &)`, `TimeSpecCondition(const ::timespec &)`, `WinFileTimeCondition(const ::FILETIME &)` and `TimeValCondition(const ::time_t &)`.

`<react/windows.hh>` defines `WinHandleCondition(HANDLE)`, and `WinHandleCondition(HANDLE, DWORD &)` if you want to get a status.

`<react/socket.hh>` defines `SocketCondition(react_sock_t, react::iomode_t)` and `WinSockCondition(SOCKET, long, long &, DWORD)`.

`<react/file.hh>` defines `FileCondition(react_file_t, react::iomode_t)`, `StdFileInCondition(FILE *)` and `StdFileOutCondition(FILE *)`.
