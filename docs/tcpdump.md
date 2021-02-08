## Observe the TCP handshake with tcpdump by GDB the code line by line

* Compile

```sh
./tcpdump_demo/build.sh
```

* Monitoring TCP port with tcpdump

```
sudo tcpdump -i any tcp port 12345
```

* GDB server

```
gdb ./debug/server
```

* `Ctrl + x + a` into TUI mode, `start` to main and the line of `int main()` will be highlighted
* input `next` or `n`, TUI will highlight the next line of code, press Enter to call `bind` and `listen`

```cpp
int listen_fd = jc::ListenFd("0.0.0.0", 12345);
```

* The status of the TCP port will change to `LISTEN`

```
netstat -antp | grep 12345
(Not all processes could be identified, non-owned process info
 will not be shown, you would have to be root to see it all.)
tcp        0      0 0.0.0.0:12345           0.0.0.0:*               LISTEN      65639/server 
```


* GDB client in another shell

```
gdb ./debug/client
```

* `Ctrl + x + a` and `start`, `n` the following line of code to call `connect`

```cpp
int fd = jc::Connect("0.0.0.0", 12345);
```

* tcpdump will display the follwing log

```
11:58:17.059466 IP localhost.56234 > localhost.12345: Flags [S], seq 2271595046, win 65495, options [mss 65495,sackOK,TS val 988640603 ecr 0,nop,wscale 7], length 0
11:58:17.068493 IP localhost.12345 > localhost.56234: Flags [S.], seq 2153659986, ack 2271595047, win 65483, options [mss 65495,sackOK,TS val 988640612 ecr 988640603,nop,wscale 7], length 0
11:58:17.068515 IP localhost.56234 > localhost.12345: Flags [.], ack 1, win 512, options [nop,nop,TS val 988640613 ecr 988640612], length 0
```

* The first column is the time, accurate to microseconds, the left side of `>` is the sender, the right side is the receiver, and `win` represents the window size (16 bits, maximum 65535). For ease of analysis, extract the key parts

```
localhost.56234 > localhost.12345: Flags [S], seq 2271595046
localhost.12345 > localhost.56234: Flags [S.], seq 2153659986, ack 2271595047
localhost.56234 > localhost.12345: Flags [.], ack 1
```

* A pair of sockets uniquely identifies a TCP connection on the network. It is a four-tuple, including the local IP address and TCP port number, destination IP address and TCP port number.
* The port number is 16 bits, ranging from 0 to 65535, and can be divided into three segments:
  * 0-1023：Well-known port, also a Unix reserved port, is a socket that can only be assigned to privileged user processes and is allocated and controlled by IANA (The Internet Assigned Numbers Authority). The same port number will be assigned to the same service, such as SSH port 22 and HTTP port 80
  * 1024-49151：registered port, which are not controlled by IANA, registered by IANA and provide a list of their usage
  * 49152-65535：dynamic or private, temporary port, 49152=65536 \* 0.75
* The captured TCP packet reflects the three-way handshake to establish a TCP connection. `S` represents `SYN`, `.` represents `ACK`, and the process is as follows
  * 1st handshake: After the server calls `bind` and `listen`, it is in the `LISTEN` state. The client calls `connect` to send a `SYN` to the server and enters the `SYN_SENT` state. The `SYN` sequence number is `2271595046`. This sequence number is a randomly generated ISN (Initial Sequence Number) that changes over time
  * 2nd handshake: After receiving `SYN`, the server sends a `SYN + ACK` to the client, and the state changes from `LISTEN` to `SYN_RCVD`. The sequence number of this `SYN` is also random, and the sequence number of `ACK` is the sequence number of the SYN sent by the client plus 1
  * 3rd handshake: After the client receives the server's `SYN + ACK`, it sends an `ACK` to the server, and the status changes from `SYN_SENT` to `ESTABLISHED`. Similarly, the sequence number of `ACK` is the sequence number of `SYN` sent by the server plus 1. It is displayed here as 1 because tcpdump uses relative sequence numbers by default. If there is no `SYN`, the increment relative to its `SYN` is displayed
  * Finally, the server receives `ACK`, and the state changes from `SYN_RCVD` to `ESTABLISHED`. Both parties enter the `ESTABLISHED` state, and the TCP connection is established
  * Except for the first active connection initiation of the three-way handshake, which sends `SYN` and `ACK` is set to 0, the `ACK` of all other TCP packets is set to 1, because the 32-bit confirmation number itself is part of the TCP header, and setting `ACK` to 1 just makes use of this part, without any additional cost
  * Each `SYN` can contain multiple TCP options. Commonly used TCP options are as follows (the latter two are called [RFC 1323](https://www.rfc-editor.org/rfc/rfc1323.html) options, also called long fat pipe options, because high bandwidth or long delay networks are called long fat pipes)
    * MSS (maximum segment size): 16 bits, with a maximum value of 65535. The end sending `SYN` notifies the other end of its maximum segment size. The Ethernet MTU is 1500 bytes. If the transmitted IP data packet is larger than the MTU, IP fragmentation is performed, and the fragments will not be reassembled before reaching the destination. The purpose of MSS is to avoid fragmentation. It is usually set to the fixed length of MTU minus IP and TCP headers. The MSS of IPv4 in Ethernet is 1500 - 20 (TCP header) - 20 (IPv4 header) = 1460. The IPv6 header is 40 bytes, corresponding to an MSS of 1440
    * Window size: 16 bits, with a maximum value of 65535. Nowadays, in order to obtain greater throughput, larger windows are required. This option specifies the number of left shift bits (0-14), and the maximum window provided is close to 1GB (65536 \* 2 ^ 14). The premise of using this option is that both end systems must support this option. The option is affected by the `SO_RVCBUF` socket option
    * Timestamps: prevents undetected corruption of the data caused by recurring packets. Programmers do not need to consider this option

* The server and client enter the `ESTABLISHED` state

```
netstat -antp | grep 12345
(Not all processes could be identified, non-owned process info
 will not be shown, you would have to be root to see it all.)
tcp        0      0 0.0.0.0:12345           0.0.0.0:*               LISTEN      65639/server 
tcp        0      0 127.0.0.1:56234         127.0.0.1:12345         ESTABLISHED 65633/client 
tcp        0      0 127.0.0.1:12345         127.0.0.1:56234         ESTABLISHED 65639/server
```

* Continue `n` in the server. `accept` is used to take the first completed connection from the `ESTABLISHED` queue and return the file descriptor of the new connection. If the queue is empty, the process is blocked

```cpp
int accept_fd = jc::AcceptFd(listen_fd);
```

* Since the client and server have established a connection, `accept` will return directly without blocking. Continue `n` with the following code to call `send`

```cpp
jc::Send(accept_fd, "welcome to join");
```

* tcpdump log

```
11:59:19.715745 IP localhost.12345 > localhost.56234: Flags [P.], seq 1:16, ack 1, win 512, options [nop,nop,TS val 988703260 ecr 988640613], length 15
11:59:19.715757 IP localhost.56234 > localhost.12345: Flags [.], ack 16, win 512, options [nop,nop,TS val 988703260 ecr 988703260], length 0
```

* `P` stands for `PSH`. When set to 1, data is sent. 16 is the data length plus 1 (the length of `welcome to join` is 15). ack is the offset. If it is sent next time, it will be 16 (the last ack + length). The ack replied by the client is the value after the colon of the sequence number. Note that these TCP packets are only generated by the server calling `send`. At this time, there is no receiver to receive the message. It can be seen that `send` does not really send the data to the other party
* Each TCP socket has a send buffer, the size of which can be changed using the `SO_SNDBUF` socket option. When an application calls `send`, the kernel copies all data from the application's buffer to the send buffer of the written socket. If the send buffer cannot hold the data, the application will be blocked until all data in the application buffer is copied to the send buffer. The return value of `send` is the length of the data put into the buffer. If it is inconsistent with the length specified in the parameter, it means that the data was not fully put in, which will result in data loss. After `send` returns, it only means that the original application buffer can be reused, and it does not mean that the other end has received the data
* Kernel parameters for `Ubuntu 18.04.5 LTS`

```
sysctl -a | grep "net.ipv4.tcp_.*mem"
net.ipv4.tcp_mem = 22062	29417	44124
net.ipv4.tcp_rmem = 4096	131072	6291456 # recv buf is 6.29 MB
net.ipv4.tcp_wmem = 4096	16384	4194304 # send buf is 4.19 MB
```

* In the client, `n` to call `recv` to get data from the buffer. TCP is a stream-oriented protocol, so it does not guarantee that the data of each `send` and `recv` will correspond. In order to reduce the number of packets sent, the sender can use the Nagel algorithm. In network programming, both parties need to specify the application layer protocol to parse the data

```cpp
jc::PrintReceiveMessage(fd);
```

* `n` the client to close the connection

```cpp
::close(fd);
```

* tcpdump log

```
12:00:42.379997 IP localhost.56234 > localhost.12345: Flags [F.], seq 1, ack 16, win 512, options [nop,nop,TS val 988785924 ecr 988703260], length 0
12:00:42.383661 IP localhost.12345 > localhost.56234: Flags [.], ack 2, win 512, options [nop,nop,TS val 988785928 ecr 988785924], length 0
```

* These are the first two of four waves to disconnect the TCP connection, where `F` stands for `FIN`
  * 1st wave: client calls `close` to send `FIN + ACK` to server and enters `FIN_WAIT_1` state
  * 2nd wave: After receiving `FIN + ACK`, the server sends an `ACK` and enters the `CLOSE_WAIT` state. This state allows the server to continue sending unfinished data. Because there may be unfinished data, the disconnection process takes one more time than the establishment of the connection. After the client receives `ACK`, the state changes from `FIN_WAIT_1` to `FIN_WAIT_2`

* The client enters the `FIN_WAIT2` state, and the server enters the `CLOSE_WAIT` state

```
netstat -antp | grep 12345
(Not all processes could be identified, non-owned process info
 will not be shown, you would have to be root to see it all.)
tcp        0      0 0.0.0.0:12345           0.0.0.0:*               LISTEN      65639/server 
tcp        0      0 127.0.0.1:56234         127.0.0.1:12345         FIN_WAIT2   -                   
tcp        1      0 127.0.0.1:12345         127.0.0.1:56234         CLOSE_WAIT  65639/server
```

* `n` the server to close the connection

```cpp
::close(accept_fd);
```

* tcpdump log

```
12:01:24.794975 IP localhost.12345 > localhost.56234: Flags [F.], seq 16, ack 2, win 512, options [nop,nop,TS val 988828339 ecr 988785924], length 0
12:01:24.794993 IP localhost.56234 > localhost.12345: Flags [.], ack 17, win 512, options [nop,nop,TS val 988828339 ecr 988828339], length 0
```

* These are the last two waves of the four waves that close the TCP connection
  * 3rd wave: the server calls `close` to send `FIN + ACK` to the client, and the state changes from `CLOSE_WAIT` to `LAST_ACK`
  * 4th wave: After receiving `FIN + ACK`, the client sends an `ACK`, and the state changes from `FIN_WAIT_2` to `TIME_WAIT`. After receiving `ACK`, the server changes its state from `LAST_ACK` to `CLOSED`. After `2MSL` (Maximum Segment Lifetime), the client state changes from `TIME_WAIT` to `CLOSED`
  * The role of `TIME_WAIT`
    * Reliably terminate TCP full-duplex connections. If the server does not receive the last `ACK` sent by the client, it will resend `FIN + ACK`, and the client will resend `ACK` and restart the `2MSL` timer
    * Allow old duplicate segments to disappear in the network. For example, if a connection is closed, another connection is established between the same IP and port soon. `TIME_WAIT` can prevent this from happening. For ports in the `TIME_WAIT` state, calling `bind` will fail, so the `SO_REUSEADDR` option is generally set when creating a socket, so that the port can be `bind` even if it is in the `TIME_WAIT` state
* Any `TCP` implementation must choose a value for MSL. [RFC 793](https://www.rfc-editor.org/rfc/rfc793.html) stipulates that MSL is 2 minutes (`For this specification the MSL is taken to be 2 minutes`). `Ubuntu 18.04.5 LTS` has a `TIME_WAIT` duration of 60 seconds, `uname -r` is `4.15.0-135-generic`, and the following code can be seen in `/usr/src/linux-headers-4.15.0-135-generic/include/net/tcp.h`

```cpp
#define TCP_TIMEWAIT_LEN                             \
  (60 * HZ) /* how long to wait to destroy TIME-WAIT \
             * state, about 60 seconds     */
#define TCP_FIN_TIMEOUT TCP_TIMEWAIT_LEN
/* BSD style FIN_WAIT2 deadlock breaker.
 * It used to be 3min, new value is 60sec,
 * to combine FIN-WAIT-2 timeout with
 * TIME-WAIT timer.
 */
```

* Kernel parameters

```
sysctl -a | grep net.ipv4.tcp_fin_timeout
net.ipv4.tcp_fin_timeout = 60
```

* At this point, the connection between the two parties has been closed. In the server, `n` the following code to close the file descriptor of the listening port

```cpp
::close(listen_fd);
```

* The above is the normal connection and disconnection process. If the client calls `connect` without running the server, the captured packets are as follows

```
12:50:57.975036 IP localhost.56262 > localhost.12345: Flags [S], seq 1183472988, win 65495, options [mss 65495,sackOK,TS val 991801517 ecr 0,nop,wscale 7], length 0
12:50:57.975051 IP localhost.12345 > localhost.56262: Flags [R.], seq 0, ack 1183472989, win 0, length 0
```

* `R` stands for `RST`. The client calls `connect` to initiate a connection. Since the port is not open, a `RST` is returned. After receiving `RST`, the client does not need to return `ACK` and directly releases the connection. The state changes to `CLOSED`. `connect` returns -1 to indicate that the call failed. If the client and server are connected normally, and the server hangs up after a while, the client will send a request to the server and will also receive a `RST` to reset the connection
* The packets captured by tcpdump can be written to a file using the following command. You can open the file with [Wireshark](https://www.wireshark.org/) to see the contents of the packet more intuitively.

```
sudo tcpdump -i any tcp port 12345 -w socket_debug.cap
```

## UDP

* UDP can send messages directly without establishing a connection. Change the TCP example to UDP and use the following command to monitor the UDP port

```
sudo tcpdump -iany udp port 12345
```

* The captured UDP packet looks like this

```
09:19:53.027405 IP localhost.39094 > localhost.12345: UDP, length 12
09:19:53.029951 IP localhost.12345 > localhost.39094: UDP, length 15
```
