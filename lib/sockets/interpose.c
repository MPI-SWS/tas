/*
 * Copyright 2019 University of Washington, Max Planck Institute for
 * Software Systems, and The University of Texas at Austin
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#define __USE_GNU
#include <dlfcn.h>
#include <pthread.h>
#include <sys/select.h>

#include <utils.h>
#include <tas_sockets.h>

static inline void ensure_init(void);

/* Function pointers to the libc functions */
static int (*libc_socket)(int domain, int type, int protocol) = NULL;
static int (*libc_close)(int sockfd) = NULL;
static int (*libc_shutdown)(int sockfd, int how) = NULL;
static int (*libc_bind)(int sockfd, const struct sockaddr *addr,
    socklen_t addrlen) = NULL;
static int (*libc_connect)(int sockfd, const struct sockaddr *addr,
    socklen_t addrlen) = NULL;
static int (*libc_listen)(int sockfd, int backlog) = NULL;
static int (*libc_accept4)(int sockfd, struct sockaddr *addr,
    socklen_t *addrlen, int flags) = NULL;
static int (*libc_accept)(int sockfd, struct sockaddr *addr,
    socklen_t *addrlen) = NULL;
static int (*libc_fcntl)(int sockfd, int cmd, ...) = NULL;
static int (*libc_getsockopt)(int sockfd, int level, int optname, void *optval,
    socklen_t *optlen) = NULL;
static int (*libc_setsockopt)(int sockfd, int level, int optname,
    const void *optval, socklen_t optlen) = NULL;
static int (*libc_getsockname)(int sockfd, struct sockaddr *addr,
    socklen_t *addrlen) = NULL;
static int (*libc_getpeername)(int sockfd, struct sockaddr *addr,
    socklen_t *addrlen) = NULL;
static ssize_t (*libc_read)(int fd, void *buf, size_t count) = NULL;
static ssize_t (*libc_recv)(int sockfd, void *buf, size_t len,
    int flags) = NULL;
static ssize_t (*libc_recvfrom)(int sockfd, void *buf, size_t len, int flags,
    struct sockaddr *src_addr, socklen_t *addrlen) = NULL;
static ssize_t (*libc_recvmsg)(int sockfd, struct msghdr *msg, int flags)
    = NULL;
static ssize_t (*libc_write)(int fd, const void *buf, size_t count) = NULL;
static ssize_t (*libc_send)(int sockfd, const void *buf, size_t len, int flags)
    = NULL;
static ssize_t (*libc_sendto)(int sockfd, const void *buf, size_t len,
    int flags, const struct sockaddr *dest_addr, socklen_t addrlen) = NULL;
static ssize_t (*libc_sendmsg)(int sockfd, const struct msghdr *msg, int flags)
    = NULL;
static int (*libc_select)(int nfds, fd_set *readfds, fd_set *writefds,
			  fd_set *exceptfds, struct timeval *timeout) = NULL;

int socket(int domain, int type, int protocol)
{
  ensure_init();

  /* if not a TCP socket, pass call to libc */
  if (domain != AF_INET || type != SOCK_STREAM) {
    return libc_socket(domain, type, protocol);
  }

  return tas_socket(domain, type, protocol);
}

int close(int sockfd)
{
  int ret;
  ensure_init();
  if ((ret = tas_close(sockfd)) == -1 && errno == EBADF) {
    return libc_close(sockfd);
  }
  return ret;
}

int shutdown(int sockfd, int how)
{
  int ret;
  ensure_init();
  if ((ret = tas_shutdown(sockfd, how)) == -1 && errno == EBADF) {
    return libc_shutdown(sockfd, how);
  }
  return ret;
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  int ret;
  ensure_init();
  if ((ret = tas_bind(sockfd, addr, addrlen)) == -1 && errno == EBADF) {
    return libc_bind(sockfd, addr, addrlen);
  }
  return ret;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  int ret;
  ensure_init();
  if ((ret = tas_connect(sockfd, addr, addrlen)) == -1 && errno == EBADF) {
    return libc_connect(sockfd, addr, addrlen);
  }
  return ret;
}

int listen(int sockfd, int backlog)
{
  int ret;
  ensure_init();
  if ((ret = tas_listen(sockfd, backlog)) == -1 && errno == EBADF) {
    return libc_listen(sockfd, backlog);
  }
  return ret;
}

int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
    int flags)
{
  int ret;
  ensure_init();
  if ((ret = tas_accept4(sockfd, addr, addrlen, flags)) == -1 &&
      errno == EBADF)
  {
    return libc_accept4(sockfd, addr, addrlen, flags);
  }
  return ret;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  int ret;
  ensure_init();
  if ((ret = tas_accept(sockfd, addr, addrlen)) == -1 && errno == EBADF) {
    return libc_accept(sockfd, addr, addrlen);
  }
  return ret;
}

int fcntl(int sockfd, int cmd, ...)
{
  int ret, arg;
  va_list val;
  ensure_init();

  va_start(val, cmd);
  arg = va_arg(val, int);
  va_end(val);

  if ((ret = tas_fcntl(sockfd, cmd, arg)) == -1 && errno == EBADF) {
    return libc_fcntl(sockfd, cmd, arg);
  }
  return ret;
}

int getsockopt(int sockfd, int level, int optname, void *optval,
    socklen_t *optlen)
{
  int ret;
  ensure_init();
  if ((ret = tas_getsockopt(sockfd, level, optname, optval, optlen)) == -1 &&
      errno == EBADF)
  {
    return libc_getsockopt(sockfd, level, optname, optval, optlen);
  }
  return ret;
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
    socklen_t optlen)
{
  int ret;
  ensure_init();
  if ((ret = tas_setsockopt(sockfd, level, optname, optval, optlen)) == -1 &&
      errno == EBADF)
  {
    return libc_setsockopt(sockfd, level, optname, optval, optlen);
  }
  return ret;
}

int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  int ret;
  ensure_init();
  if ((ret = tas_getsockname(sockfd, addr, addrlen)) == -1 && errno == EBADF) {
    return libc_getsockname(sockfd, addr, addrlen);
  }
  return ret;
}

int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  int ret;
  ensure_init();
  if ((ret = tas_getpeername(sockfd, addr, addrlen)) == -1 && errno == EBADF) {
    return libc_getpeername(sockfd, addr, addrlen);
  }
  return ret;
}

ssize_t read(int sockfd, void *buf, size_t count)
{
  ssize_t ret;
  ensure_init();
  if ((ret = tas_read(sockfd, buf, count)) == -1 && errno == EBADF) {
    return libc_read(sockfd, buf, count);
  }
  return ret;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
  ssize_t ret;
  ensure_init();
  if ((ret = tas_recv(sockfd, buf, len, flags)) == -1 && errno == EBADF) {
    return libc_recv(sockfd, buf, len, flags);
  }
  return ret;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
    struct sockaddr *src_addr, socklen_t *addrlen)
{
  ssize_t ret;
  ensure_init();
  if ((ret = tas_recvfrom(sockfd, buf, len, flags, src_addr, addrlen)) == -1 &&
      errno == EBADF)
  {
    return libc_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
  }
  return ret;
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
  ssize_t ret;
  ensure_init();
  if ((ret = tas_recvmsg(sockfd, msg, flags)) == -1 && errno == EBADF) {
    return libc_recvmsg(sockfd, msg, flags);
  }
  return ret;
}

ssize_t write(int sockfd, const void *buf, size_t count)
{
  ssize_t ret;
  ensure_init();
  if ((ret = tas_write(sockfd, buf, count)) == -1 && errno == EBADF) {
    return libc_write(sockfd, buf, count);
  }
  return ret;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
  ssize_t ret;
  ensure_init();
  if ((ret = tas_send(sockfd, buf, len, flags)) == -1 && errno == EBADF) {
    return libc_send(sockfd, buf, len, flags);
  }
  return ret;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                   const struct sockaddr *dest_addr, socklen_t addrlen)
{
  ssize_t ret;
  ensure_init();
  if ((ret = tas_sendto(sockfd, buf, len, flags, dest_addr, addrlen)) == -1 &&
      errno == EBADF)
  {
    return libc_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
  }
  return ret;
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
  ssize_t ret;
  ensure_init();
  if ((ret = tas_sendmsg(sockfd, msg, flags)) == -1 && errno == EBADF) {
    return libc_sendmsg(sockfd, msg, flags);
  }
  return ret;
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
    struct timeval *timeout)
{
  return tas_select(nfds, readfds, writefds, exceptfds, timeout);
}

int epoll_create(int size)
{
  return tas_epoll_create(size);
}

int epoll_create1(int flags)
{
  return tas_epoll_create1(flags);
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
  return tas_epoll_ctl(epfd, op, fd, event);
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents,
    int timeout)
{
  return tas_epoll_wait(epfd, events, maxevents, timeout);
}

int epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
    int timeout, const sigset_t *sigmask)
{
  return tas_epoll_pwait(epfd, events, maxevents, timeout, sigmask);
}

/******************************************************************************/
/* Helper functions */

static void *bind_symbol(const char *sym)
{
  void *ptr;
  if ((ptr = dlsym(RTLD_NEXT, sym)) == NULL) {
    fprintf(stderr, "flextcp socket interpose: dlsym failed (%s)\n", sym);
    abort();
  }
  return ptr;
}

static void init(void)
{
  libc_socket = bind_symbol("socket");
  libc_close = bind_symbol("close");
  libc_shutdown = bind_symbol("shutdown");
  libc_bind = bind_symbol("bind");
  libc_connect = bind_symbol("connect");
  libc_listen = bind_symbol("listen");
  libc_accept4 = bind_symbol("accept4");
  libc_accept = bind_symbol("accept");
  libc_fcntl = bind_symbol("fcntl");
  libc_getsockopt = bind_symbol("getsockopt");
  libc_setsockopt = bind_symbol("setsockopt");
  libc_getsockname = bind_symbol("getsockname");
  libc_getpeername = bind_symbol("getpeername");
  libc_read = bind_symbol("read");
  libc_recv = bind_symbol("recv");
  libc_recvfrom = bind_symbol("recvfrom");
  libc_recvmsg = bind_symbol("recvmsg");
  libc_write = bind_symbol("write");
  libc_send = bind_symbol("send");
  libc_sendto = bind_symbol("sendto");
  libc_sendmsg = bind_symbol("sendmsg");
  libc_select = bind_symbol("select");

  if (tas_init() != 0) {
    abort();
  }
}

static inline void ensure_init(void)
{
  static volatile uint32_t init_cnt = 0;
  static volatile uint8_t init_done = 0;
  static __thread uint8_t in_init = 0;

  if (init_done == 0) {
    /* during init the socket functions will be used to connect to the kernel on
     * a unix socket, so make sure that runs through. */
    if (in_init) {
      return;
    }

    if (__sync_fetch_and_add(&init_cnt, 1) == 0) {
      in_init = 1;
      init();
      in_init = 0;
      MEM_BARRIER();
      init_done = 1;
    } else {
      while (init_done == 0) {
        pthread_yield();
      }
      MEM_BARRIER();
    }
  }
}
