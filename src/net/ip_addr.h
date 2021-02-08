#pragma once

#include <netinet/in.h>

#include <string>

namespace jc {

class IPAddr {
 public:
  constexpr IPAddr() = default;
  constexpr IPAddr(const sockaddr_in& addr) : addr_(addr) {}
  IPAddr(std::string_view ip, uint16_t port);
  constexpr const sockaddr_in& SocketAddr() const { return addr_; }

  std::string IP() const;
  uint16_t Port() const;
  std::string IPPort() const;

 private:
  sockaddr_in addr_ = {};
};

}  // namespace jc
