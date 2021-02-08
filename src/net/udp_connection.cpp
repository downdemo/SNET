#include "net/udp_connection.h"

#include "net/socket_utils.h"

namespace jc {

UDPConnection::UDPConnection(const IPAddr& local_addr, const IPAddr& peer_addr,
                             std::string_view nic)
    : local_addr_(local_addr), peer_addr_(peer_addr) {
  fd_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd_ == -1) {
    printf("failed to create udp socket fd\n");
    exit(1);
  }
  if (!nic.empty()) {
    socket_bind_nic(fd_, nic);
  }
  if (!socket_bind(fd_, local_addr_.SocketAddr())) {
    printf("failed to bind addr=%s\n", local_addr_.IPPort().c_str());
    exit(1);
  }
}

UDPConnection::~UDPConnection() { ::close(fd_); }

int UDPConnection::Send(char* data, std::size_t len,
                        const IPAddr& peer_addr) const {
  return ::sendto(fd_, data, len, 0,
                  reinterpret_cast<const sockaddr*>(&peer_addr.SocketAddr()),
                  sizeof(peer_addr.SocketAddr()));
}

int UDPConnection::Send(char* data, std::size_t len) const {
  return Send(data, len, peer_addr_);
}

std::pair<int, IPAddr> UDPConnection::Recv(char* data, std::size_t len) const {
  sockaddr_in peer_addr = {};
  socklen_t peer_addr_len = sizeof(peer_addr);
  int res = ::recvfrom(fd_, data, len, 0,
                       reinterpret_cast<sockaddr*>(&peer_addr), &peer_addr_len);
  return std::make_pair(res, IPAddr{peer_addr});
}

}  // namespace jc
