#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <string_view>

namespace jc {

sockaddr_in SockAddress(std::string_view ip, uint16_t port) {
  sockaddr_in res = {};
  res.sin_family = AF_INET;
  res.sin_port = ::htons(port);
  ::inet_pton(AF_INET, ip.data(), &res.sin_addr);
  return res;
}

int ListenFd(std::string_view ip, uint16_t port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == -1) {
    std::cerr << "errno: " << strerror(errno) << std::endl;
    return -1;
  }
  int reuse = 1;
  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
    std::cerr << "errno: " << strerror(errno) << std::endl;
    return -1;
  }
  sockaddr_in address = SockAddress(ip, port);
  if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) ==
      -1) {
    std::cerr << "errno: " << strerror(errno) << std::endl;
    return -1;
  }
  if (::listen(fd, SOMAXCONN) == -1) {
    std::cerr << "errno: " << strerror(errno) << std::endl;
    return -1;
  }
  return fd;
}

int AcceptFd(int listen_fd) {
  sockaddr_in client;
  socklen_t client_address_length = sizeof(client);
  int accept_fd = ::accept(listen_fd, reinterpret_cast<sockaddr*>(&client),
                           &client_address_length);
  if (accept_fd == -1) {
    std::cout << "errno: " << strerror(errno) << std::endl;
    return -1;
  }
  std::cout << "receive connect request from " << inet_ntoa(client.sin_addr)
            << ':' << ::ntohs(client.sin_port) << std::endl;
  return accept_fd;
}

void Send(int accept_fd, std::string_view s) {
  ::send(accept_fd, s.data(), s.size(), 0);
}

}  // namespace jc

int main() {
  int listen_fd = jc::ListenFd("0.0.0.0", 12345);
  assert(listen_fd != -1);
  int accept_fd = jc::AcceptFd(listen_fd);
  assert(accept_fd != -1);
  jc::Send(accept_fd, "welcome to join");
  ::close(accept_fd);
  ::close(listen_fd);
}
