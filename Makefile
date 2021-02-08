$(shell if [ -e lib ]; then rm -rf lib; fi; mkdir lib)
$(shell if [ -e bin ]; then rm -rf bin; fi; mkdir bin)
CXX = g++
CXXFLAGS = -std=c++20 -Wall
LDFLAGS = -Isrc -Llib -lsnet -lpthread -Wl,-rpath=lib
BUILD_TYPE=Release

ifeq ($(BUILD_TYPE), Release)
    CXXFLAGS += -O2 -DNDEBUG
else
    CXXFLAGS += -O0 -g
endif

all:  libsnet.so \
	test_epoll_connection \
	test_epoll_ping_pong \
	test_epoll_timer \
	test_poll_connection \
	test_poll_ping_pong \
	test_poll_timer \
	test_select_connection \
	test_select_ping_pong \
	test_select_timer \
	test_tcp_connection \
	test_tcp_ping_pong \
	test_udp_recv \
	test_tcp_fork \

libsnet.so: src/net/*.cpp
	$(CXX) $(CXXFLAGS) -shared -fpic -Isrc src/net/*.cpp -o lib/libsnet.so

test_epoll_connection: test/test_epoll_connection.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_epoll_connection.cpp $(LDFLAGS) -o bin/test_epoll_connection

test_epoll_ping_pong: test/test_epoll_ping_pong.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_epoll_ping_pong.cpp $(LDFLAGS) -o bin/test_epoll_ping_pong

test_epoll_timer: test/test_epoll_timer.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_epoll_timer.cpp $(LDFLAGS) -o bin/test_epoll_timer

test_poll_connection: test/test_poll_connection.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_poll_connection.cpp $(LDFLAGS) -o bin/test_poll_connection

test_poll_ping_pong: test/test_poll_ping_pong.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_poll_ping_pong.cpp $(LDFLAGS) -o bin/test_poll_ping_pong

test_poll_timer: test/test_poll_timer.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_poll_timer.cpp $(LDFLAGS) -o bin/test_poll_timer

test_select_connection: test/test_select_connection.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_select_connection.cpp $(LDFLAGS) -o bin/test_select_connection

test_select_ping_pong: test/test_select_ping_pong.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_select_ping_pong.cpp $(LDFLAGS) -o bin/test_select_ping_pong

test_select_timer: test/test_select_timer.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_select_timer.cpp $(LDFLAGS) -o bin/test_select_timer

test_tcp_connection: test/test_tcp_connection.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_tcp_connection.cpp $(LDFLAGS) -o bin/test_tcp_connection

test_tcp_ping_pong: test/test_tcp_ping_pong.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_tcp_ping_pong.cpp $(LDFLAGS) -o bin/test_tcp_ping_pong

test_udp_recv: test/test_udp_recv.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_udp_recv.cpp $(LDFLAGS) -o bin/test_udp_recv

test_tcp_fork: test/test_tcp_fork.cpp libsnet.so
	$(CXX) $(CXXFLAGS) test/test_tcp_fork.cpp $(LDFLAGS) -o bin/test_tcp_fork

clean:
	rm -rf lib
	rm -rf bin
