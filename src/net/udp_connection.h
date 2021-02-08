#pragma once

#include <memory>
#include <string>

#include "base/noncopyable.h"
#include "net/ip_addr.h"

namespace jc {

class UDPConnection : noncopyable {
 public:
  UDPConnection(const IPAddr& local_addr, const IPAddr& peer_addr = {},
                std::string_view nic = "");
  ~UDPConnection();
  constexpr const IPAddr& LocalAddr() const { return local_addr_; }
  constexpr const IPAddr& PeerAddr() const { return peer_addr_; }
  int Send(char* data, std::size_t len, const IPAddr& peer_addr) const;
  int Send(char* data, std::size_t len) const;
  std::pair<int, IPAddr> Recv(char* data, std::size_t len) const;

 private:
  int fd_ = -1;
  IPAddr local_addr_ = {};
  IPAddr peer_addr_ = {};
};

}  // namespace jc
