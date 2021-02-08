#include <signal.h>

#include <thread>

#include "net/poller_poll.h"

namespace jc {

class Timer : public PollerPoll<Timer> {
 public:
  void OnRead(int fd) { exit(1); }
  void OnWrite(int fd) { exit(1); }
};

template <uint64_t timeout_millsec>
class Tester {
 public:
  void run() {
    std::jthread t{[&] {
      while (millsec_ < timeout_millsec) {
        ++index_;
        // if no yield or sleep timer always print 0
        std::this_thread::yield();
      }
    }};
    Timer timer;
    timer.AddTimer(
        [&] {
          millsec_ += 500;
          if (millsec_ >= timeout_millsec) {
            timer.Exit();
          }
        },
        500);
    timer.AddTimer([&] { printf("index=%ld\n", index_); }, 100);
    timer.Loop();
  }

 private:
  uint64_t index_ = 0;
  uint64_t millsec_ = 0;
};

}  // namespace jc

int main() {
  ::signal(SIGCHLD, SIG_IGN);
  ::signal(SIGPIPE, SIG_IGN);
  jc::Tester<3000>{}.run();
}
