## Socket API

### [socket(2)](http://man.he.net/man2/socket)

* 为了执行网络 I/O，一个进程必须做的第一件事就是创建 socket 描述符。指定期望的协议类型，成功时返回一个小的非负整数值

```cpp
#include <sys/socket.h>
#include <sys/types.h>

int socket(int domain, int type, int protocal);
```

* domain：协议族，一般是 `AF_INET`，它决定了 socket 地址类型，如 `AF_INET` 决定要用 `32 位 IPv4 地址和 16 位端口号` 组合，常用的协议族有 `AF_INET(IPv4)、AF_INET6(IPv6)、AF_LOCAL（或称 AF_UNIX，Unix 域 socket）、AF_ROUTE`
* type：socket 类型，一般是 `SOCK_STREAM`，即 TCP，常用的有 `SOCK_STREAM（TCP）、SOCK_DGRAM（UDP）、SOCK_RAW（原始套接字）、SOCK_PACKET、SOCK_SEQPACKET`
* protocol：指定协议，为 0 时自动选择 type 类型对应的协议，常用的有 `IPPROTO_TCP、IPPTOTO_UDP、IPPROTO_SCTP、IPPROTO_TIPC`，注意 type 和 protocol 必须匹配，不能随意组合
* 示例

```cpp
int sock = socket(AF_INET, SOCK_STREAM, 0);
```

### [bind(2)](http://man.he.net/man2/bind)

* 给 socket 描述符绑定一个协议地址。如果调用 connect 或 listen 之前未用 bind 捆绑一个端口，内核就会为描述符分配一个临时端口，一般 TCP 客户端会采用这种做法，而 TCP 服务器应该在 listen 前 bind

```cpp
#include <sys/socket.h>
#include <sys/types.h>

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

* 示例

```cpp
sockaddr_in address;  // #include <netinet/in.h>
address.sin_family = AF_INET;
address.sin_port = htons(12345);
address.sin_addr.s_addr = htonl(INADDR_ANY);
bzero(&(address.sin_zero), 8);
bind(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address));
```

* sockaddr_in 定义如下

```cpp
struct sockaddr_in {
  short sin_family;         // 2 bytes e.g. AF_INET, AF_INET6
  unsigned short sin_port;  // 2 bytes e.g. htons(3490)
  struct in_addr sin_addr;  // 4 bytes see struct in_addr, below
  char sin_zero[8];         // 8 bytes zero this if you want to
};

struct in_addr {
  unsigned long s_addr;  // 4 bytes load with inet_pton()
};

// sockaddr 和 sockaddr_in 大小都是 16 字节，只不过 sockaddr_in 把 14 个字节的
// sa_data 拆开了 sin_zero 用于填充字节，保证 sockaddr_in 和 sockaddr 一样大
struct sockaddr {
  unsigned short sa_family;  // 2 bytes address family, AF_xxx
  char sa_data[14];          // 14 bytes of protocol address
};

// sockaddr 是给系统用的，程序员应该用 sockaddr_in
// 通常用类型、IP 地址、端口填充 sockaddr_in 后，转换成 sockaddr
// 作为参数传递给调用函数
```

### [listen(2)](http://man.he.net/man2/listen)

* 仅由 TCP 服务器调用。用 socket 创建的套接字会被假设为主动套接字，即调用 connect 的客户端套接字。listen 将主动套接字转换成被动套接字，将套接字从 CLOSED 状态转换到 LISTEN 状态

```cpp
#include <sys/socket.h>
#include <sys/types.h>

int listen(int sockfd, int backlog);
```

* backlog：套接字（包括 SYS_RCVD 和 ESTABLISHED 队列）最大连接数，如果达到上限，客户端将收到 `ECONNREFUSED` 错误，一般设为 5
* listen 只适用于 `SOCK_STREAM` 和 `SOCK_SEQPACKET` 的 socket 类型，协议族为 `AF_INET` 时 `backlog` 最大可设为 128
* 示例

```cpp
listen(sock, 5);
```

### [accept(2)](http://man.he.net/man2/accept)

* 由 TCP 服务器调用，用于从 `ESTABLISHED` 队列头返回下一个已完成连接，如果队列为空则进程阻塞

```cpp
#include <sys/socket.h>
#include <sys/types.h>

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```
* 第一个参数为服务端的监听描述符，accept 成功时返回一个自动生成的客户端描述符。监听描述符在服务器的生命期内一直存在，客户端描述符在服务器完成对给定客户的服务时关闭
* addr 和 addrlen 用来保存已连接的客户端的协议地址
* addrlen 调用前为 addr 所指的套接字地址结构的长度，返回时为由内核存放在该套接字地址结构内的确切字节数
* 示例

```cpp
sockaddr_in client_address;
socklen_t len = sizeof(client_address);
int client_fd =
    accept(sock, reinterpret_cast<sockaddr*>(&client_address), &len);
if (connectFd != -1) {
  std::cout << "ip: " << inet_ntoa(client_address.sin_addr) << std::endl;
  std::cout << "port: " << ntohs(client_address.sin_port) << std::endl;
  close(client_fd);
}
```

### [connect(2)](http://man.he.net/man2/connect)

* 由 TCP 客户端调用，参数与 bind 相同，connect 前可以不 bind，内核会确定源 IP 地址并选择一个临时端口。如果是 TCP 套接字，调用 connect 将触发 TCP 的三路握手，且仅在连接成功或出错时才返回

```cpp
#include <sys/socket.h>
#include <sys/types.h>

int connect(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen);
```

* 示例

```cpp
sockaddr_in server;
bzero(&server, sizeof(server));
server.sin_family = AF_INET;
server.sin_port = htons(12345);
server.sin_addr.s_addr = inet_addr("192.168.211.129");
connect(sock, reinterpret_cast<sockaddr*>(&server), sizeof(server));
```

### [close(2)](http://man.he.net/man2/close)

* 关闭套接字，终止 TCP 连接。并发服务器中父进程关闭已连接套接字只是导致相应描述符的引用计数值减 1，仅在该计数变为 0 时才关闭套接字

```cpp
#include <unistd.h>

int close(int sockfd);
```

* 示例

```cpp
close(sock);
```

### 字符处理函数

* 字节操控函数

```cpp
#include <strings.h>

void bzero(void *s, size_t n);
void bcopy(const void *src, void *dest, size_t n);
int bcmp(const void *s1, const void *s2, size_t n);  // 相等返回 0，否则返回非 0
```

* 网络字节序通常为大端字节序，主机字节序通常为小端字节序。以下函数提供了主机字节序和网络字节序之间的转换

```cpp
#include <arpa/inet.h>

// 主机字节序转网络字节序
unit16_t htons(unit16_t hostshort);  // 一般用于端口号
uint32_t htonl(uint32_t hostlong);   // 一般用于 IP 地址

// 网络字节序转主机字节序
uint16_t ntohs(uint16_t netshort);  // 一般用于端口号
uint32_t ntohl(uint32_t netlong);   // 一般用于 IP 地址
```

* 点分十进制字符串（即 `xxx.xxx.xxx.xxx`）转网络字节序

```cpp
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// 将 cp 转换为 32 位网络字节序二进制值并存储在 inp 中。字符串有效返回
// 1，否则返回 0
int inet_aton(const char *cp, struct in_addr *inp);

// 将点分十进制字符串 cp 转为 IPv4 地址，字符串有效则返回 32
// 位二进制网络字节序的 IPv4 地址 否则为 INADDR_NONE，通常是 -1，32 位均为 1，即
// 255.255.255.255，这也意味着不能处理 255.255.255.255 现在 inet_addr 已被废弃
in_addr_t inet_addr(const char *cp);

// 返回指向点分十进制字符串的指针，返回值所指向的字符串驻留在静态内存中，因此该函数是不可重入的
char *inet_ntoa(struct in_addr in);
```

* [inet_pton](http://man.he.net/man3/inet_pton) 和 [inet_ntop](http://man.he.net/man3/inet_ntop) 是随 IPv6 出现的新函数，对 IPv4 和 IPv6 都适用

```cpp
#include <arpa/inet.h>

// 成功返回 1，输入不是有效的表达式返回 0，出错返回 -1
int inet_pton(int af, const char *src, void *dst);

// 成功返回指向结果的指针，出错返回 NULL
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

// <netinet/in.h> 中有如下定义，可用作 size
// 如果 size 不足以容纳表达式结果则返回一个空指针并置 errno 为 ENOSPC
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
```

* inet_pton 和 inet_ntop 示例

```cpp
const char* ip = "192.168.211.129";
// foo.sin_add.s_addr = inet_addr(ip);
inet_pton(AF_INET, ip, &foo.sin_addr);

char s[INET_ADDRSTRLEN];
// const char* p = inet_ntoa(foo.sin_addr);
const char* p = inet_ntop(AF_INET, &foo.sin_addr, s, sizeof(s));
```

### [fork(2)](man.he.net/man2/fork)

* fork 创建一个新的子进程，在子进程中返回值为 0，在父进程中返回值为子进程的 PID，在子进程中可以通过 [getppid](http://man.he.net/man2/getppid) 获取父进程的 PID，如果 fork 调用失败，则返回 -1

```cpp
#include <sys/types.h>
#include <unistd.h>

pid_t fork(void);
```

* 子进程和父进程继续执行 fork 调用之后的指令，但不确定先后顺序。子进程是父进程的副本，子进程不共享父进程的数据空间和堆栈，不过由于 fork 之后经常紧跟 exec，所以现在很多实现并不执行一个完全的副本，而是使用写时复制（Copy-On-Write，COW）技术，让父子空间共享区域，并将它们的访问权限改变为只读，如果任何一个父子进程要修改这些区域，内核会只为修改区域的那块内存制作一个副本，这个副本通常是虚拟内存中的一页

```cpp
int i = 0;
pid_t pid = fork();

if (pid == -1) {
  std::cerr << "errno: " << errno << std::endl;
} else if (pid == 0) {
  std::cout << "child id: " << getpid() << std::endl;
  std::cout << "parent id:" << getppid() << std::endl;
  ++i;
} else {
  std::cout << "child id: " << pid << std::endl;
  std::cout << "parent id: " << getpid() << std::endl;
}

std::cout << i;  // 在子进程中为 1，在父进程中为 0
```

* fork 有两个典型用法
  * 一个进程创建一个自身的副本，这样每个副本都可以在另一个副本执行其他任务的同时处理各自的某个操作，这是网络服务器的典型用法
  * 一个进程想要执行另一个程序，先 fork 一个自身的副本，然后其中一个副本调用 exec 把自身替换成新的程序，这是 Shell 之类的程序的典型用法
