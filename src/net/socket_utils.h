#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <utility>

namespace jc {

inline sockaddr_in resolve_socketaddr(std::string_view host, uint16_t port) {
  hostent* he = ::gethostbyname(host.data());
  if (!he) {
    printf("failed to resolve: %s\n", host.data());
    exit(1);
  }
  assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
  sockaddr_in res = {};
  res.sin_family = AF_INET;
  res.sin_port = ::htons(port);
  res.sin_addr = *reinterpret_cast<in_addr*>(he->h_addr);
  return res;
}

inline std::string sockaddr_to_ip(const sockaddr_in& addr) {
  char res[INET_ADDRSTRLEN] = {};
  ::inet_ntop(AF_INET, &addr.sin_addr, res, sizeof(res));
  return res;  // return ::inet_ntoa(addr.sin_addr);
}

inline uint16_t sockaddr_to_port(const sockaddr_in& addr) {
  return be16toh(addr.sin_port);  // ::ntohs
}

inline std::string sockaddr_to_ip_port(const sockaddr_in& addr) {
  return sockaddr_to_ip(addr) + ":" + std::to_string(sockaddr_to_port(addr));
}

inline bool socket_set_nonblocking(int fd) {  // evutil_make_socket_nonblocking
  int old_option = ::fcntl(fd, F_GETFL);
  int new_option = old_option | O_NONBLOCK;
  return ::fcntl(fd, F_SETFL, new_option) >= 0;
}

inline bool socket_set_reuseaddr(int fd) {
  int flag = 1;
  return !::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}

inline bool socket_set_keepalive(int fd) {
  int flag = 1;
  return !::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag));
}

inline bool socket_disable_nagle(int fd) {
  int flag = 1;
  return !::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

inline bool socket_set_connect_retry_times(int fd, int n) {
  int flag = n;  // timeout is 2 ^ (n + 1) - 1 seconds
  return !::setsockopt(fd, IPPROTO_TCP, TCP_SYN_SENT, &flag, sizeof(flag));
}

inline bool socket_bind(int fd, const sockaddr_in& addr) {
  return ::bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) !=
         -1;
}

inline bool socket_bind_nic(int fd, std::string_view nic) {
  ifreq ifr = {};
  strcpy(ifr.ifr_name, nic.data());
  return !::setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
                       reinterpret_cast<char*>(&ifr), sizeof(ifr));
}

inline bool socket_listen(int fd) { return ::listen(fd, SOMAXCONN) != -1; }

inline int create_listen_fd(std::string_view ip, uint16_t port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == -1) {
    printf("failed to create tcp socket fd\n");
    exit(1);
  }
  if (!socket_set_reuseaddr(fd)) {
    printf("failed to set_reuseaddr for fd=%d\n", fd);
    exit(1);
  }
  sockaddr_in addr = resolve_socketaddr(ip.data(), port);
  if (!socket_bind(fd, addr)) {
    printf("failed to bind addr=%s\n", sockaddr_to_ip_port(addr).c_str());
    exit(1);
  }
  if (!socket_listen(fd)) {
    printf("failed to listen addr=%s\n", sockaddr_to_ip_port(addr).c_str());
    exit(1);
  }
  return fd;
}

inline int socket_accept(int fd, sockaddr_in& peer_addr) {
  socklen_t peer_addr_len = sizeof(peer_addr);
  return ::accept(fd, reinterpret_cast<sockaddr*>(&peer_addr), &peer_addr_len);
}

inline bool socket_connect(int fd, const sockaddr_in& addr) {
  socket_set_connect_retry_times(fd, 2);
  return ::connect(fd, reinterpret_cast<const sockaddr*>(&addr),
                   sizeof(addr)) != -1;
}

inline bool socket_shutdown(int fd) { return ::shutdown(fd, SHUT_WR) >= 0; }

inline uint16_t get_free_port() {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    exit(1);
  }
  sockaddr_in addr = {};
  socklen_t addr_len = sizeof(addr);
  if (socket_set_reuseaddr(fd) && socket_bind(fd, addr) &&
      !::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &addr_len)) {
    ::close(fd);
    return ::ntohs(addr.sin_port);
  }
  ::close(fd);
  printf("failed to get_free_port\n");
  exit(1);
}

inline int create_epfd() {
  int fd = ::epoll_create1(EPOLL_CLOEXEC);
  if (fd == -1) {
    printf("failed to create epfd\n");
    exit(1);
  }
  return fd;
}

inline bool epfd_add_read(int epfd, int fd) {
  epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLIN;
  return ::epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) != -1 &&
         socket_set_nonblocking(fd);
}

inline bool epfd_add_write(int epfd, int fd) {
  epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLOUT | EPOLLET;
  return ::epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) != -1 &&
         socket_set_nonblocking(fd);
}

inline bool epfd_mod(int epfd, int fd, int op) {
  epoll_event ev;
  ev.data.fd = fd;
  ev.events = op;
  return ::epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) != -1;
}

inline bool epfd_mod_read(int epfd, int fd) {
  return epfd_mod(epfd, fd, EPOLLIN);
}

inline bool epfd_mod_write(int epfd, int fd) {
  return epfd_mod(epfd, fd, EPOLLOUT | EPOLLET);
}

inline bool epfd_del(int epfd, int fd) {
  return ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) != -1;
}

inline int create_tfd(uint64_t millsec) {
  int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (fd == -1) {
    printf("failed to create timerfd\n");
    exit(1);
  }
  itimerspec ts = {};
  ts.it_interval.tv_sec = millsec / 1000;
  ts.it_interval.tv_nsec = (millsec % 1000) * 1000000;
  ts.it_value = ts.it_interval;
  if (::timerfd_settime(fd, 0, &ts, nullptr) == -1) {
    printf("failed to set time for timerfd\n");
    ::close(fd);
    exit(1);
  }
  return fd;
}

inline void read_tfd(int tfd) {
  uint64_t howmany;
  if (::read(tfd, &howmany, sizeof(howmany)) != sizeof(howmany)) {
    printf("read tfd error\n");
  }
}

}  // namespace jc
