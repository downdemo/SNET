#pragma once

#include <sys/select.h>

#include <algorithm>
#include <functional>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "base/noncopyable.h"
#include "net/socket_utils.h"

namespace jc {

template <typename Derived>
class PollerSelect : noncopyable {
 public:
  ~PollerSelect();
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
  struct Event {
    int fd;
    bool is_read;
  };
  bool is_running_ = false;
  int max_fd_ = -1;
  fd_set event_read_;
  fd_set event_write_;
  std::vector<Event> events_;
  std::unordered_set<int> fds_removed_;
  std::unordered_map<int, std::function<void()>> tfd_2_cb_;
};

template <typename Derived>
inline PollerSelect<Derived>::~PollerSelect() {
  is_running_ = false;
  std::ranges::for_each(tfd_2_cb_, [](const auto& x) { ::close(x.first); });
  printf("Desctuctor PollerSelect\n");
};

template <typename Derived>
inline void PollerSelect<Derived>::Loop() {
  timeval tv;
  tv.tv_sec = 10;
  tv.tv_usec = 0;
  is_running_ = true;
  while (is_running_) {
    ResetEvent();
    int ret = ::select(max_fd_ + 1, &event_read_, &event_write_, nullptr, &tv);
    if (ret == 0 || errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
      continue;
    }
    if (ret < 0) {
      printf("failed to select, errno[%d]=%s\n", errno, strerror(errno));
      exit(1);
    }
    std::size_t nfds = events_.size();
    std::ranges::for_each_n(events_.begin(), nfds, [&](Event& event) {
      int fd = event.fd;
      if (event.is_read && FD_ISSET(fd, &event_read_)) {
        if (tfd_2_cb_.contains(fd)) {
          read_tfd(fd);
          std::invoke(tfd_2_cb_.at(fd));
        } else {
          static_cast<Derived*>(this)->OnRead(fd);
        }
      } else if (!event.is_read && FD_ISSET(fd, &event_write_)) {
        static_cast<Derived*>(this)->OnWrite(fd);
      }
    });
  }
};

template <typename Derived>
inline void PollerSelect<Derived>::Exit() {
  is_running_ = false;
};

template <typename Derived>
inline void PollerSelect<Derived>::AddRead(int fd) {
  socket_set_nonblocking(fd);
  FD_SET(fd, &event_read_);
  events_.emplace_back(fd, true);
};

template <typename Derived>
inline void PollerSelect<Derived>::AddWrite(int fd) {
  socket_set_nonblocking(fd);
  FD_SET(fd, &event_write_);
  events_.emplace_back(fd, false);
};

template <typename Derived>
inline void PollerSelect<Derived>::SetRead(int fd) {
  std::ranges::for_each(events_, [&](Event& event) {
    if (event.fd == fd) {
      event.is_read = true;
    }
  });
};

template <typename Derived>
inline void PollerSelect<Derived>::SetWrite(int fd) {
  std::ranges::for_each(events_, [&](Event& event) {
    if (event.fd == fd) {
      event.is_read = false;
    }
  });
};

template <typename Derived>
inline void PollerSelect<Derived>::Remove(int fd) {
  FD_CLR(fd, &event_read_);
  FD_CLR(fd, &event_write_);
  fds_removed_.emplace(fd);
  ::close(fd);
  tfd_2_cb_.erase(fd);
};

template <typename Derived>
inline void PollerSelect<Derived>::AddTimer(const std::function<void()>& f,
                                            uint64_t millsec) {
  if (!f) {
    return;
  }
  int tfd = create_tfd(millsec);
  tfd_2_cb_.try_emplace(tfd, f);
  AddRead(tfd);
};

template <typename Derived>
inline void PollerSelect<Derived>::ResetEvent() {
  FD_ZERO(&event_read_);
  FD_ZERO(&event_write_);
  std::erase_if(events_, [&](const Event& event) {
    return fds_removed_.contains(event.fd);
  });
  fds_removed_.clear();
  max_fd_ = -1;
  std::ranges::for_each(events_, [&](const Event& event) {
    max_fd_ = std::max(event.fd, max_fd_);
    FD_SET(event.fd, event.is_read ? &event_read_ : &event_write_);
  });
}

}  // namespace jc
