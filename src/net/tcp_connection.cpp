#include "net/tcp_connection.h"

namespace jc {

TCPConnection::TCPConnection(int fd, const IPAddr& local_addr,
                             const IPAddr& peer_addr)
    : fd_(fd), local_addr_(local_addr), peer_addr_(peer_addr) {
  socket_disable_nagle(fd_);
  socket_set_keepalive(fd_);
}

TCPConnection::~TCPConnection() {
  Shutdown();
  ::close(fd_);
}

int TCPConnection::Send(char* data, std::size_t len) const {
  return ::send(fd_, data, len, 0);
}

int TCPConnection::Recv(char* data, std::size_t len) const {
  return ::recv(fd_, data, len, 0);
}

void TCPConnection::Shutdown() const { socket_shutdown(fd_); }

}  // namespace jc
