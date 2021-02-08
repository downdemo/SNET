#include "net/tcp_client.h"

#include "net/socket_utils.h"

namespace jc {

TCPClient::TCPClient(std::string_view peer_ip, uint16_t peer_port,
                     std::string_view local_ip, uint16_t local_port)
    : peer_addr_(peer_ip, peer_port), local_addr_(local_ip, local_port) {
  fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd_ == -1) {
    printf("failed to create tcp socket fd\n");
    exit(1);
  }
  if (!socket_bind(fd_, local_addr_.SocketAddr())) {
    printf("failed to bind addr=%s\n", local_addr_.IPPort().c_str());
    exit(1);
  }
}

TCPClient::~TCPClient() { ::close(fd_); }

std::unique_ptr<TCPConnection> TCPClient::Connect() const {
  if (!socket_connect(fd_, peer_addr_.SocketAddr())) {
    printf("failed to connect %s to %s\n", local_addr_.IPPort().c_str(),
           peer_addr_.IPPort().c_str());
    return nullptr;
  }
  return std::make_unique<TCPConnection>(fd_, local_addr_.SocketAddr(),
                                         peer_addr_.SocketAddr());
}

}  // namespace jc
