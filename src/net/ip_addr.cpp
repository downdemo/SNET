#include "net/ip_addr.h"

#include "net/socket_utils.h"

namespace jc {

IPAddr::IPAddr(std::string_view ip, uint16_t port)
    : addr_(resolve_socketaddr(ip, port)) {}

std::string IPAddr::IP() const { return sockaddr_to_ip(addr_); }

uint16_t IPAddr::Port() const { return sockaddr_to_port(addr_); }

std::string IPAddr::IPPort() const { return sockaddr_to_ip_port(addr_); }

}  // namespace jc
