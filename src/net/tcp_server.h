#pragma once

#include <memory>
#include <string>

#include "base/noncopyable.h"
#include "net/ip_addr.h"
#include "net/tcp_connection.h"

namespace jc {

class TCPServer : noncopyable {
 public:
  TCPServer(std::string_view ip, uint16_t port);
  ~TCPServer();
  std::unique_ptr<TCPConnection> Accept() const;
  constexpr int FD() const { return fd_; }

 private:
  int fd_ = -1;
  IPAddr addr_ = {};
};

}  // namespace jc
