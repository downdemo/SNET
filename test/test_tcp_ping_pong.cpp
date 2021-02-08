#include <signal.h>

#include <chrono>
#include <thread>

#include "net/tcp_client.h"
#include "net/tcp_connection.h"
#include "net/tcp_server.h"

namespace jc {

template <uint64_t timeout_millsec>
class Tester {
 public:
  void run() {
    is_running_ = true;
    t_server_ = std::jthread{&Tester::run_server, this};
    t_client_ = std::jthread{&Tester::run_client, this};
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_millsec));
    is_running_ = false;
    t_server_.join();
    t_client_.join();
  }

 private:
  void run_server() {
    TCPServer server{"localhost", port_};
    auto connection = server.Accept();
    int i = 0;
    while (is_running_) {
      int len = connection->Recv(buf_, sizeof(buf_));
      printf("connection[1] server[%s] recv from client[%s], msg[%d]=%s\n",
             connection->PeerAddr().IPPort().c_str(),
             connection->LocalAddr().IPPort().c_str(), len, buf_);
      std::string msg = std::string{pong_msg_} + std::to_string(++i);
      connection->Send(const_cast<char*>(msg.c_str()), msg.size());
    }
  }

  void run_client() {
    TCPClient client{"localhost", port_, "localhost", get_free_port()};
    auto connection = client.Connect();
    if (!connection) {
      exit(1);
    }
    int i = 0;
    while (is_running_) {
      std::string msg = std::string{ping_msg_} + std::to_string(++i);
      connection->Send(const_cast<char*>(msg.c_str()), msg.size());
      int len = connection->Recv(buf_, sizeof(buf_));
      printf("connection[1] client[%s] recv from server[%s], msg[%d]=%s\n",
             connection->LocalAddr().IPPort().c_str(),
             connection->PeerAddr().IPPort().c_str(), len, buf_);
    }
  }

 private:
  const uint16_t port_ = get_free_port();
  static constexpr std::string_view ping_msg_ = "ping";
  static constexpr std::string_view pong_msg_ = "pong";
  char buf_[1024] = {};
  bool is_running_ = false;
  std::jthread t_server_;
  std::jthread t_client_;
};

}  // namespace jc

int main() {
  ::signal(SIGCHLD, SIG_IGN);
  ::signal(SIGPIPE, SIG_IGN);
  jc::Tester<3000>{}.run();
}
