#include <signal.h>

#include <chrono>
#include <thread>

#include "net/poller_test.h"
#include "net/tcp_client.h"
#include "net/tcp_server.h"

namespace jc {

template <uint64_t timeout_millsec>
class Tester {
 public:
  void run() {
    is_running_ = true;
    std::jthread t{&Tester::run_client, this};
    PollerTCPServer<PollerPoll>{"localhost", port_, timeout_millsec};
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout_millsec));
    is_running_ = false;
  }

 private:
  void run_client() {
    int i = 0;
    while (is_running_) {
      TCPClient client{"localhost", port_, "localhost", get_free_port()};
      auto connection = client.Connect();
      if (!connection) {
        exit(1);
      }
      ++i;
      std::string msg = std::string{ping_msg_} + std::to_string(i);
      connection->Send(const_cast<char*>(msg.c_str()), msg.size());
      int len = connection->Recv(buf_, sizeof(buf_));
      printf("connection[%d] client[%s] recv from server[%s], msg[%d]=%s\n", i,
             connection->LocalAddr().IPPort().c_str(),
             connection->PeerAddr().IPPort().c_str(), len, buf_);
    }
  }

 private:
  static constexpr std::string_view ping_msg_ = "ping";
  char buf_[1024] = {};
  bool is_running_ = false;
  const uint16_t port_ = get_free_port();
};

}  // namespace jc

int main() {
  ::signal(SIGCHLD, SIG_IGN);
  ::signal(SIGPIPE, SIG_IGN);
  jc::Tester<3000>{}.run();
}
