#pragma once

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <vector>

#include "base/noncopyable.h"
#include "net/socket_utils.h"

namespace jc {

template <typename Derived>
class PollerEpoll : noncopyable {
 public:
  ~PollerEpoll();
  void Loop();
  void Exit();
  void AddRead(int fd) const;
  void AddWrite(int fd) const;
  void SetRead(int fd) const;
  void SetWrite(int fd) const;
  void Remove(int fd);
  void AddTimer(const std::function<void()>& f, uint64_t millsec);

 private:
  int epfd_ = create_epfd();
  bool is_running_ = false;
  std::vector<epoll_event> events_;
  std::unordered_map<int, std::function<void()>> tfd_2_cb_;
};

template <typename Derived>
inline PollerEpoll<Derived>::~PollerEpoll() {
  is_running_ = false;
  ::close(epfd_);
  std::ranges::for_each(tfd_2_cb_, [](const auto& x) { ::close(x.first); });
  printf("Desctuctor PollerEpoll\n");
};

template <typename Derived>
inline void PollerEpoll<Derived>::Loop() {
  events_.resize(16);
  is_running_ = true;
  while (is_running_) {
    int ret = ::epoll_wait(epfd_, events_.data(), events_.size(), 10000);
    if (ret == 0 || errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
      continue;
    }
    if (ret < 0) {
      printf("failed to epoll_wait, errno[%d]=%s\n", errno, strerror(errno));
      exit(1);
    }
    std::ranges::for_each_n(events_.begin(), ret, [&](epoll_event& event) {
      int fd = event.data.fd;
      if (tfd_2_cb_.contains(fd)) {
        read_tfd(fd);
        std::invoke(tfd_2_cb_.at(fd));
      } else if (event.events & EPOLLIN) {
        static_cast<Derived*>(this)->OnRead(fd);
      } else if (event.events & EPOLLOUT) {
        static_cast<Derived*>(this)->OnWrite(fd);
      } else {
        printf("unsupport epoll event=%d\n", event.events);
        exit(1);
      }
    });
    if (static_cast<std::size_t>(ret) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  }
};

template <typename Derived>
inline void PollerEpoll<Derived>::Exit() {
  is_running_ = false;
};

template <typename Derived>
inline void PollerEpoll<Derived>::AddRead(int fd) const {
  epfd_add_read(epfd_, fd);
};

template <typename Derived>
inline void PollerEpoll<Derived>::AddWrite(int fd) const {
  epfd_add_write(epfd_, fd);
};

template <typename Derived>
inline void PollerEpoll<Derived>::SetRead(int fd) const {
  epfd_mod_read(epfd_, fd);
};

template <typename Derived>
inline void PollerEpoll<Derived>::SetWrite(int fd) const {
  epfd_mod_write(epfd_, fd);
};

template <typename Derived>
inline void PollerEpoll<Derived>::Remove(int fd) {
  epfd_del(epfd_, fd);
  ::close(fd);
  tfd_2_cb_.erase(fd);
};

template <typename Derived>
inline void PollerEpoll<Derived>::AddTimer(const std::function<void()>& f,
                                           uint64_t millsec) {
  if (!f) {
    return;
  }
  int tfd = create_tfd(millsec);
  tfd_2_cb_.try_emplace(tfd, f);
  AddRead(tfd);
};

}  // namespace jc
