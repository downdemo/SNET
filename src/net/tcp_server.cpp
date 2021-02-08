#include "net/tcp_server.h"

#include "net/socket_utils.h"

namespace jc {

TCPServer::TCPServer(std::string_view ip, uint16_t port) : addr_(ip, port) {
  fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd_ == -1) {
    printf("failed to create tcp socket fd\n");
    exit(1);
  }
  if (!socket_set_reuseaddr(fd_)) {
    printf("failed to set_reuseaddr for fd=%d\n", fd_);
    exit(1);
  }
  if (!socket_bind(fd_, addr_.SocketAddr())) {
    printf("failed to bind addr=%s\n", addr_.IPPort().c_str());
    exit(1);
  }
  if (!socket_listen(fd_)) {
    printf("failed to listen addr=%s\n", addr_.IPPort().c_str());
    exit(1);
  }
}

TCPServer::~TCPServer() { ::close(fd_); }

std::unique_ptr<TCPConnection> TCPServer::Accept() const {
  sockaddr_in peer_addr;
  int conn_fd = socket_accept(fd_, peer_addr);
  if (conn_fd == -1) {
    printf("failed to accept addr=%s\n", addr_.IPPort().c_str());
    exit(1);
  }
  return std::make_unique<TCPConnection>(conn_fd, IPAddr(peer_addr),
                                         addr_.SocketAddr());
}

}  // namespace jc
