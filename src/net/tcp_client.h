#pragma once

#include <memory>
#include <string>

#include "base/noncopyable.h"
#include "net/ip_addr.h"
#include "net/tcp_connection.h"

namespace jc {

class TCPClient : noncopyable {
 public:
  TCPClient(std::string_view peer_ip, uint16_t peer_port,
            std::string_view local_ip = "localhost", uint16_t local_port = 0);
  ~TCPClient();
  constexpr int FD() const { return fd_; }
  std::unique_ptr<TCPConnection> Connect() const;

 private:
  int fd_ = -1;
  IPAddr peer_addr_ = {};
  IPAddr local_addr_ = {};
};

}  // namespace jc
