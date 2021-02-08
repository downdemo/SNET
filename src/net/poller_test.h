#pragma once

#include <concepts>
#include <unordered_map>

#include "base/concepts.h"
#include "net/poller_epoll.h"
#include "net/poller_poll.h"
#include "net/poller_select.h"
#include "net/tcp_client.h"
#include "net/tcp_server.h"

namespace jc {

template <template <typename> class Poller>
requires(is_same_template_v<Poller, PollerEpoll> ||
         is_same_template_v<Poller, PollerPoll> ||
         is_same_template_v<Poller, PollerSelect>) class PollerTCPServer
    : public Poller<PollerTCPServer<Poller>> {
 public:
  PollerTCPServer(std::string_view ip, uint16_t port,
                  uint64_t timeout_millsec) {
    server_ = std::make_unique<TCPServer>(ip, port);
    this->AddRead(server_->FD());
    if (timeout_millsec != 0) {
      this->AddTimer([&] { this->Exit(); }, timeout_millsec);
    }
    this->Loop();
  }

  void OnRead(int fd) {
    static int conn_cnt = 0;
    if (fd == server_->FD()) {
      auto connection = server_->Accept();
      ++conn_cnt;
      int conn_fd = connection->FD();
      this->AddRead(conn_fd);
      fd_2_connections_[conn_fd] = std::move(connection);
      fd_2_index_[conn_fd] = conn_cnt;
      return;
    }
    if (fd_2_connections_.contains(fd)) {
      auto& connection = fd_2_connections_.at(fd);
      int len = connection->Recv(buf_, sizeof(buf_));
      if (len == 0) {
        this->Remove(fd);
        fd_2_connections_.erase(fd);
        fd_2_index_.erase(fd);
        return;
      }
      printf("connection[%d] server[%s] recv from client[%s], msg[%d]=%s\n",
             fd_2_index_.at(fd), connection->PeerAddr().IPPort().c_str(),
             connection->LocalAddr().IPPort().c_str(), len, buf_);
      std::string msg = std::string{pong_msg_} + std::to_string(++index_);
      connection->Send(const_cast<char*>(msg.c_str()), msg.size());
    }
  }

  void OnWrite(int fd) { exit(1); }

 private:
  static constexpr std::string_view pong_msg_ = "pong";

 private:
  std::unique_ptr<TCPServer> server_;
  std::unordered_map<int, std::unique_ptr<TCPConnection>> fd_2_connections_;
  std::unordered_map<int, int> fd_2_index_;
  char buf_[1024] = {};
  int index_ = 0;
};

template <template <typename> class Poller>
requires(is_same_template_v<Poller, PollerEpoll> ||
         is_same_template_v<Poller, PollerPoll> ||
         is_same_template_v<Poller, PollerSelect>) class PollerTCPClient
    : public Poller<PollerTCPClient<Poller>> {
 public:
  PollerTCPClient(std::string_view ip, uint16_t port,
                  uint64_t timeout_millsec) {
    client_ =
        std::make_unique<TCPClient>(ip, port, "localhost", get_free_port());
    do {
      connection_ = client_->Connect();
    } while (!connection_);
    this->AddWrite(connection_->FD());
    if (timeout_millsec != 0) {
      this->AddTimer([&] { this->Exit(); }, timeout_millsec);
    }
    this->Loop();
  }

  void OnRead(int fd) {
    if (fd != connection_->FD()) {
      return;
    }
    int len = connection_->Recv(buf_, sizeof(buf_));
    if (len == 0) {
      printf("OnDisconnected %s from %s\n",
             connection_->LocalAddr().IPPort().c_str(),
             connection_->PeerAddr().IPPort().c_str());
      this->Exit();
      return;
    }
    printf("connection[1] client[%s] recv from server[%s], msg[%d]=%s\n",
           connection_->LocalAddr().IPPort().c_str(),
           connection_->PeerAddr().IPPort().c_str(), len, buf_);
    this->SetWrite(fd);
  }

  void OnWrite(int fd) {
    if (fd != connection_->FD()) {
      return;
    }
    std::string msg = std::string{ping_msg_} + std::to_string(++index_);
    connection_->Send(const_cast<char*>(msg.c_str()), msg.size());
    this->SetRead(fd);
  }

 private:
  static constexpr std::string_view ping_msg_ = "ping";

 private:
  std::unique_ptr<TCPClient> client_;
  std::unique_ptr<TCPConnection> connection_;
  char buf_[1024] = {};
  int index_ = 0;
};

}  // namespace jc
