#include <signal.h>

#include <chrono>
#include <thread>

#include "net/socket_utils.h"
#include "net/udp_connection.h"

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
    UDPConnection connection{IPAddr{"localhost", port_}};
    while (is_running_) {
      auto [len, client_addr] = connection.Recv(buf_, sizeof(buf_));
      printf("server[%s] recv client[%s] msg[%d]=%s\n",
             connection.LocalAddr().IPPort().c_str(),
             client_addr.IPPort().c_str(), len, buf_);
    }
  }

  void run_client() {
    IPAddr peer_addr{"localhost", port_};
    IPAddr local_addr{"localhost", 0};
    int i = 0;
    while (is_running_) {
      UDPConnection connection{local_addr, peer_addr, "lo"};
      std::string msg = std::string{ping_msg_} + std::to_string(++i);
      connection.Send(const_cast<char*>(msg.c_str()), msg.size());
    }
  }

 private:
  const uint16_t port_ = get_free_port();
  static constexpr std::string_view ping_msg_ = "ping";
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
