#pragma once

#include <string>

#include "base/noncopyable.h"
#include "net/ip_addr.h"
#include "net/socket_utils.h"

namespace jc {

class TCPConnection : noncopyable {
 public:
  TCPConnection(int fd, const IPAddr& local_addr, const IPAddr& peer_addr);
  ~TCPConnection();
  constexpr int FD() const { return fd_; }
  constexpr const IPAddr& LocalAddr() const { return local_addr_; }
  constexpr const IPAddr& PeerAddr() const { return peer_addr_; }
  int Send(char* data, std::size_t len) const;
  int Recv(char* data, std::size_t len) const;
  void Shutdown() const;

 private:
  int fd_ = -1;
  IPAddr local_addr_ = {};
  IPAddr peer_addr_ = {};
};

}  // namespace jc
