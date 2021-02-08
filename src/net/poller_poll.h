#pragma once

#include <sys/poll.h>

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/noncopyable.h"
#include "net/socket_utils.h"

namespace jc {

template <typename Derived>
class PollerPoll : noncopyable {
 public:
  ~PollerPoll();
  void Loop();
  void Exit();
  void AddRead(int fd);
  void AddWrite(int fd);
  void SetRead(int fd);
  void SetWrite(int fd);
  void Remove(int fd);
  void AddTimer(const std::function<void()>& f, uint64_t millsec);

 private:
  void ResetEvent();

 private:
  bool is_running_ = false;
  std::vector<pollfd> events_;
  std::unordered_set<int> fds_removed_;
  std::unordered_map<int, std::function<void()>> tfd_2_cb_;
};

template <typename Derived>
inline PollerPoll<Derived>::~PollerPoll() {
  is_running_ = false;
  std::ranges::for_each(tfd_2_cb_, [](const auto& x) { ::close(x.first); });
  printf("Desctuctor PollerPoll\n");
};

template <typename Derived>
inline void PollerPoll<Derived>::Loop() {
  is_running_ = true;
  while (is_running_) {
    ResetEvent();
    int ret = ::poll(events_.data(), events_.size(), 10000);
    if (ret == 0 || errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
      continue;
    }
    if (ret < 0) {
      printf("failed to poll, errno[%d]=%s\n", errno, strerror(errno));
      exit(1);
    }
    std::size_t pfdn = events_.size();
    std::ranges::for_each_n(events_.begin(), pfdn, [&](pollfd& pfd) {
      int fd = pfd.fd;
      if (pfd.revents & POLLIN) {
        if (tfd_2_cb_.contains(fd)) {
          read_tfd(fd);
          std::invoke(tfd_2_cb_.at(fd));
        } else {
          static_cast<Derived*>(this)->OnRead(fd);
        }
      } else if (pfd.revents & POLLOUT) {
        static_cast<Derived*>(this)->OnWrite(fd);
      }
    });
  }
};

template <typename Derived>
inline void PollerPoll<Derived>::Exit() {
  is_running_ = false;
};

template <typename Derived>
inline void PollerPoll<Derived>::AddRead(int fd) {
  socket_set_nonblocking(fd);
  pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  events_.emplace_back(std::move(pfd));
};

template <typename Derived>
inline void PollerPoll<Derived>::AddWrite(int fd) {
  socket_set_nonblocking(fd);
  pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLOUT;
  events_.emplace_back(std::move(pfd));
};

template <typename Derived>
inline void PollerPoll<Derived>::SetRead(int fd) {
  std::ranges::for_each(events_, [&](pollfd& pfd) {
    if (pfd.fd == fd) {
      pfd.events = POLLIN;
    }
  });
};

template <typename Derived>
inline void PollerPoll<Derived>::SetWrite(int fd) {
  std::ranges::for_each(events_, [&](pollfd& pfd) {
    if (pfd.fd == fd) {
      pfd.events = POLLOUT;
    }
  });
};

template <typename Derived>
inline void PollerPoll<Derived>::Remove(int fd) {
  fds_removed_.emplace(fd);
  ::close(fd);
  tfd_2_cb_.erase(fd);
};

template <typename Derived>
inline void PollerPoll<Derived>::AddTimer(const std::function<void()>& f,
                                          uint64_t millsec) {
  if (!f) {
    return;
  }
  int tfd = create_tfd(millsec);
  tfd_2_cb_.try_emplace(tfd, f);
  AddRead(tfd);
};

template <typename Derived>
inline void PollerPoll<Derived>::ResetEvent() {
  std::erase_if(events_, [&](const pollfd& pfd) {
    return fds_removed_.contains(pfd.fd);
  });
  fds_removed_.clear();
}

}  // namespace jc
