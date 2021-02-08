## [select(2)](http://man.he.net/man2/select)

* 调用 select 函数会让内核轮询 fd_set，直到某个 fd_set 有就绪的文件描述时返回，内核标记该文件描述符所在的位，未就绪的位均置零。传入的第一个参数表示内核对 fd_set 轮询的位数，轮询范围应该为 0 到最大的文件描述符，因此第一个参数一般设置为最大文件描述符加 1

```cpp
#include <sys/select.h>
#include <sys/time.h>

/* Check the first NFDS descriptors each in READFDS (if not NULL) for read
   readiness, in WRITEFDS (if not NULL) for write readiness, and in EXCEPTFDS
   (if not NULL) for exceptional conditions.  If TIMEOUT is not NULL, time out
   after waiting the interval specified therein.  Returns the number of ready
   descriptors, or -1 for errors.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int select(int __nfds, fd_set *__restrict __readfds,
                  fd_set *__restrict __writefds, fd_set *__restrict __exceptfds,
                  struct timeval *__restrict __timeout);
```

* 如果设置了超时时间，即使没有文件描述符就绪，也会在超时时间返回 0。虽然 timeval 可以设置到微秒级，但内核的实际支持往往粗糙得多，一般 Unix 内核会往 10 ms 的倍数向上取整

```cpp
/* A time value that is accurate to the nearest
   microsecond but also has a range of years.  */
struct timeval {
  __time_t tv_sec;       /* Seconds.  */
  __suseconds_t tv_usec; /* Microseconds.  */
};
```

* fd_set 的本质是一个 long int 数组

```cpp
/* The fd_set member is required to be an array of longs.  */
typedef long int __fd_mask;

/* Some versions of <linux/posix_types.h> define this macros.  */
#undef __NFDBITS
/* It's easier to assume 8-bit bytes than to get CHAR_BIT.  */
#define __NFDBITS (8 * (int)sizeof(__fd_mask))
#define __FD_ELT(d) ((d) / __NFDBITS)
#define __FD_MASK(d) ((__fd_mask)(1UL << ((d) % __NFDBITS)))

/* fd_set for select and pselect.  */
typedef struct {
  /* XPG4.2 requires this member name.  Otherwise avoid the name
     from the global namespace.  */
#ifdef __USE_XOPEN
  __fd_mask fds_bits[__FD_SETSIZE / __NFDBITS];
#define __FDS_BITS(set) ((set)->fds_bits)
#else
  __fd_mask __fds_bits[__FD_SETSIZE / __NFDBITS];
#define __FDS_BITS(set) ((set)->__fds_bits)
#endif
} fd_set;
```

* 大小为 1024 位

```cpp
/* Number of descriptors that can fit in an `fd_set'.  */
#define __FD_SETSIZE 1024
```

* 因此 fd_set 也相当于一个 bit_map，FD_SET 函数根据文件描述符的值，将对应的 fd_set 位置 1

```cpp
void FD_SET(int fd, fd_set* fdset);
```

* 当 select 返回时，仅表示有文件描述符就绪，但不知道是哪一个，因此还需要再次遍历所有文件描述符做 FD_ISSET 检查。另外，select 会修改传入的参数，因此下一次调用 select 时仍要重新设置 fd_set 和 timeout（timeout 会被清零，如果不重新设置，没有描述符就绪就会直接超时）。可以通过如下四个函数操作 fd_set，其实现本质是位运算，比如 FD_SET 先除以数组每个元素的大小找出新的 fd 所在的下标，再将 1 左移余数数量位的结果与该元素求或

```cpp
/* Access macros for `fd_set'.  */
#define FD_SET(fd, fdsetp) __FD_SET(fd, fdsetp)
#define FD_CLR(fd, fdsetp) __FD_CLR(fd, fdsetp)
#define FD_ISSET(fd, fdsetp) __FD_ISSET(fd, fdsetp)
#define FD_ZERO(fdsetp) __FD_ZERO(fdsetp)

#define __FD_SET(d, set) ((void)(__FDS_BITS(set)[__FD_ELT(d)] |= __FD_MASK(d)))
#define __FD_CLR(d, set) ((void)(__FDS_BITS(set)[__FD_ELT(d)] &= ~__FD_MASK(d)))
#define __FD_ISSET(d, set) ((__FDS_BITS(set)[__FD_ELT(d)] & __FD_MASK(d)) != 0)

#if defined __GNUC__ && __GNUC__ >= 2

#if __WORDSIZE == 64
#define __FD_ZERO_STOS "stosq"
#else
#define __FD_ZERO_STOS "stosl"
#endif

#define __FD_ZERO(fdsp)                                                     \
  do {                                                                      \
    int __d0, __d1;                                                         \
    __asm__ __volatile__("cld; rep; " __FD_ZERO_STOS                        \
                         : "=c"(__d0), "=D"(__d1)                           \
                         : "a"(0), "0"(sizeof(fd_set) / sizeof(__fd_mask)), \
                           "1"(&__FDS_BITS(fdsp)[0])                        \
                         : "memory");                                       \
  } while (0)

#else /* ! GNU CC */

/* We don't use `memset' because this would require a prototype and
   the array isn't too big.  */
#define __FD_ZERO(set)                                             \
  do {                                                             \
    unsigned int __i;                                              \
    fd_set *__arr = (set);                                         \
    for (__i = 0; __i < sizeof(fd_set) / sizeof(__fd_mask); ++__i) \
      __FDS_BITS(__arr)[__i] = 0;                                  \
  } while (0)

#endif /* GNU CC */

#define __NFDBITS (8 * (int)sizeof(__fd_mask))
#define __FD_ELT(d) ((d) / __NFDBITS)
#define __FD_MASK(d) ((__fd_mask)(1UL << ((d) % __NFDBITS)))
```

* 通常一个 select 函数的调用过程如下

```cpp
// 准备多个 fd，保存到一个 fds 数组中，最大的记为 max_fd
fd_set read_fds;
while (true) {
  FD_ZERO(&read_fds);
  FD_SET(fds[0], &read_fds);
  FD_SET(fds[1], &read_fds);
  FD_SET(fds[2], &read_fds);
  timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  int ret = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
  if (ret == -1) {
    std::cout << "errno: " << strerror(errno) << std::endl;
    break;
  }
  if (ret == 0) {
    std::cout << "timeout" << std::endl;
    continue;
  }
  for (int i = 0; i < fd_count; ++i) {
    if (FD_ISSET(fds[i], &read_fds)) {
      ...
    }
  }
}
```

* select 的本质是中断，CPU 在每个指令周期结束后检查中断寄存器，如果发生中断则保存当前上下文再转而处理中断。中断本质是硬件轮询，比起直接用代码轮询（软件层面的轮询，本质是通过跳转指令实现），CPU 效率更高，时间成本几乎为 0

## [poll(2)](http://man.he.net/man2/poll)

* poll 的原理和 select 相同，只不过参数类型有一些区别，poll 轮询的数据结构是元素类型为 pollfd 的数组（因此监听的文件描述符不仅限于 1024 个），nfds 是元素个数，timeout 是一个表示毫秒的整型

```cpp
#include <poll.h>

int poll(struct pollfd *__fds, nfds_t __nfds, int __timeout);

struct pollfd {
  int fd;            /* File descriptor to poll.  */
  short int events;  /* Types of events poller cares about.  */
  short int revents; /* Types of events that actually occurred.  */
};

/* Event types that can be polled for.  These bits may be set in `events'
   to indicate the interesting event types; they will appear in `revents'
   to indicate the status of the file descriptor.  */
#define POLLIN 0x001  /* There is data to read.  */
#define POLLPRI 0x002 /* There is urgent data to read.  */
#define POLLOUT 0x004 /* Writing now will not block.  */

#if defined __USE_XOPEN || defined __USE_XOPEN2K8
/* These values are defined in XPG4.2.  */
#define POLLRDNORM 0x040 /* Normal data may be read.  */
#define POLLRDBAND 0x080 /* Priority data may be read.  */
#define POLLWRNORM 0x100 /* Writing now will not block.  */
#define POLLWRBAND 0x200 /* Priority data may be written.  */
#endif

#ifdef __USE_GNU
/* These are extensions for Linux.  */
#define POLLMSG 0x400
#define POLLREMOVE 0x1000
#define POLLRDHUP 0x2000
#endif

/* Event types always implicitly polled for.  These bits need not be set in
   `events', but they will appear in `revents' to indicate the status of
   the file descriptor.  */
#define POLLERR 0x008  /* Error condition.  */
#define POLLHUP 0x010  /* Hung up.  */
#define POLLNVAL 0x020 /* Invalid polling request.  */
```

* 通常 poll 函数的调用过程如下

```cpp
std::vector<pollfd> pollfds(1);
pollfds[0].fd = listen_fd;
pollfds[0].events = POLLIN;

while (true) {
  constexpr int timeout = 1000;
  int ret = poll(pollfds.data(), pollfds.size(), timeout);
  if (ret == -1) {
    std::cout << "errno: " << strerror(errno) << std::endl;
    break;
  }
  if (ret == 0) {
    std::cout << "timeout" << std::endl;
    continue;
  }
  for (int i = 0; i < pollfds.size(); ++i) {
    if (pollfds[i].revents & POLLIN) {
      ...
    }
  }
}
```

## [epoll(7)](http://man.he.net/man7/epoll)

* epoll 在 Linux 2.5.44 中首次出现，使用前需要先创建一个 epfd，内核会分配一个对应的 eventpoll

```cpp
/* Creates an epoll instance.  Returns an fd for the new instance.
   The "size" parameter is a hint specifying the number of file
   descriptors to be associated with the new instance.  The fd
   returned by epoll_create() should be closed with close().  */
extern int epoll_create(int __size) __THROW;

/* Same as epoll_create but with an FLAGS parameter.  The unused SIZE
   parameter has been dropped.  */
extern int epoll_create1(int __flags) __THROW;
```

* 用文件描述符生成一个 epoll_event 对象

```cpp
typedef union epoll_data {
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event {
  uint32_t events;   /* Epoll events */
  epoll_data_t data; /* User data variable */
} __EPOLL_PACKED;
```

* 随后将 epoll_event 添加到 epfd 中，eventpoll 以红黑树的数据结构存储 epitem，因此 epoll_ctl 的时间复杂度为 `O(log n)`。每个文件描述符只会有一次从用户态到内核的拷贝，不存在 select 和 poll 的重复拷贝的情况，[随着文件描述符数量增加，select 和 poll 的响应速度将线性下降，而 epoll 基本不受影响](http://www.xmailserver.org/linux-patches/nio-improve.html)。epoll_ctl 的实现中是有锁的，因此是线程安全的

```cpp
/* Valid opcodes ( "op" parameter ) to issue to epoll_ctl().  */
#define EPOLL_CTL_ADD 1 /* Add a file descriptor to the interface.  */
#define EPOLL_CTL_DEL 2 /* Remove a file descriptor from the interface.  */
#define EPOLL_CTL_MOD 3 /* Change file descriptor epoll_event structure.  */

/* Manipulate an epoll instance "epfd". Returns 0 in case of success,
   -1 in case of error ( the "errno" variable will contain the
   specific error code ) The "op" parameter is one of the EPOLL_CTL_*
   constants defined above. The "fd" parameter is the target of the
   operation. The "event" parameter describes which events the caller
   is interested in and any associated user data.  */
extern int epoll_ctl(int __epfd, int __op, int __fd,
                     struct epoll_event *__event) __THROW;
```

* 事件发生时，对应的文件描述符会被添加到一个链表中，epoll_wait 直接遍历链表即可获取所有就绪的文件描述符。epoll_wait 和 epoll_ctl 一样是线程安全的

```cpp
/* Wait for events on an epoll instance "epfd". Returns the number of
   triggered events returned in "events" buffer. Or -1 in case of
   error with the "errno" variable set to the specific error code. The
   "events" parameter is a buffer that will contain triggered
   events. The "maxevents" is the maximum number of events to be
   returned ( usually size of "events" ). The "timeout" parameter
   specifies the maximum wait time in milliseconds (-1 == infinite).

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern int epoll_wait(int __epfd, struct epoll_event *__events, int __maxevents,
                      int __timeout);
```

* epoll 可以注册如下事件类型，其中 EPOLLET 表示边缘触发（Edge Triggered）模式，即 epoll 检查到事件触发后必须立即处理，下次不会再通知 epoll_wait，ET 减少了 epoll 事件重复触发的次数，结合 non-blocking 非常高效。如果不设置 ET，则默认使用电平触发（Level Triggered）模式，即此次未被处理的事件下次会继续通知 epoll_wait，直到被处理。LT 适合读数据，数据没读完下次会继续提醒，ET 适合写数据和接受数据

```cpp
enum EPOLL_EVENTS {
  EPOLLIN = 0x001,
#define EPOLLIN EPOLLIN
  EPOLLPRI = 0x002,
#define EPOLLPRI EPOLLPRI
  EPOLLOUT = 0x004,
#define EPOLLOUT EPOLLOUT
  EPOLLRDNORM = 0x040,
#define EPOLLRDNORM EPOLLRDNORM
  EPOLLRDBAND = 0x080,
#define EPOLLRDBAND EPOLLRDBAND
  EPOLLWRNORM = 0x100,
#define EPOLLWRNORM EPOLLWRNORM
  EPOLLWRBAND = 0x200,
#define EPOLLWRBAND EPOLLWRBAND
  EPOLLMSG = 0x400,
#define EPOLLMSG EPOLLMSG
  EPOLLERR = 0x008,
#define EPOLLERR EPOLLERR
  EPOLLHUP = 0x010,
#define EPOLLHUP EPOLLHUP
  EPOLLRDHUP = 0x2000,
#define EPOLLRDHUP EPOLLRDHUP
  EPOLLEXCLUSIVE = 1u << 28,
#define EPOLLEXCLUSIVE EPOLLEXCLUSIVE
  EPOLLWAKEUP = 1u << 29,
#define EPOLLWAKEUP EPOLLWAKEUP
  EPOLLONESHOT = 1u << 30,
#define EPOLLONESHOT EPOLLONESHOT
  EPOLLET = 1u << 31
#define EPOLLET EPOLLET
};
```

* 通常 epoll 的调用过程如下

```cpp
int epfd = epoll_create1(0);
epoll_event ev;
ev.data.fd = fd;
ev.events = EPOLLIN | EPOLLET;
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);  // SetNonBlocking
std::vector<epoll_event> v(1000);
while (true) {
  int ret = epoll_wait(epfd, v.data(), v.size(), -1);
  if (ret == -1) {
    std::cout << "errno: " << strerror(errno) << std::endl;
    break;
  }
  for (int i = 0; i < ret; ++i) {
    if (v[i].data.fd == listen_fd) {
      ...  // do_accept
    } else if (v[i].events & EPOLLIN) {
      ...  // do_read
    } else if (v[i].events & EPOLLOUT) {
      ...  // do_write
    }
  }
}
```
