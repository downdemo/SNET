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

int Connect(std::string_view ip, uint16_t port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == -1) {
    std::cerr << "errno: " << strerror(errno) << std::endl;
    return -1;
  }
  sockaddr_in address = SockAddress(ip, port);
  if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) ==
      -1) {
    std::cerr << "errno: " << strerror(errno) << std::endl;
    return -1;
  }
  return fd;
}

bool PrintReceiveMessage(int fd) {
  char buf[1024] = {};
  int n = ::recv(fd, buf, sizeof(buf) - 1, 0);
  if (n <= 0) {
    return false;
  }
  std::cout << n << " bytes message: " << buf << std::endl;
  return true;
}

}  // namespace jc

int main() {
  int fd = jc::Connect("0.0.0.0", 12345);
  assert(fd != -1);
  jc::PrintReceiveMessage(fd);
  ::close(fd);
}
