#include "Socket.hpp"

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

Socket::Socket() noexcept : fd_(-1) {
}
Socket::Socket(int fd) noexcept : fd_(fd) {
}
Socket::~Socket() {
  close();
}

Socket::Socket(Socket && other) noexcept : fd_(other.fd_) {
  other.fd_ = -1;
}
Socket & Socket::operator=(Socket && other) noexcept {
  if (this != &other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
  }
  return *this;
}

int Socket::get_fd() const noexcept {
  return fd_;
}
bool Socket::valid() const noexcept {
  return fd_ >= 0;
}

void Socket::close() noexcept {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

std::size_t Socket::recvSome(char * buf, std::size_t len) const {
  for (;;) {
    ssize_t n = ::recv(fd_, buf, len, 0);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      throw std::runtime_error(std::string("recv failed: ") + std::strerror(errno));
    }
    return static_cast<std::size_t>(n);
  }
}

void Socket::recvAll(char * buf, std::size_t len) const {
  std::size_t total_received = 0;
  while (total_received < len) {
    size_t bytes = recvSome(buf + total_received, len - total_received);
    if (bytes == 0) {
      throw std::runtime_error("Peer closed connection before all data was received");
    }
    total_received += bytes;
  }
}

Socket Socket::createListeningSocket(std::uint16_t port) {
  Socket s;
  s.listen(port);
  return s;
}

void Socket::listen(std::uint16_t port) {
  addrinfo hints{}, *res;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  std::string port_str = std::to_string(port);

  if(::getaddrinfo(nullptr, port_str.c_str(), &hints, &res) != 0) {
    throw std::runtime_error("getaddrinfo failed");
  }

  int fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    ::freeaddrinfo(res);
    throw std::runtime_error("socket creation failed");
  }
  fd_ = fd;

  int yes = 1;
  if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    ::freeaddrinfo(res);
    ::close(fd_);
    fd_ = -1;
    throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
  }

  if (::bind(fd_, res->ai_addr, res->ai_addrlen) < 0) {
    ::freeaddrinfo(res);
    ::close(fd_);
    fd_ = -1;
    throw std::runtime_error("bind failed");
  }

  ::freeaddrinfo(res);

  if (::listen(fd_, 50) < 0) {
    ::close(fd_);
    fd_ = -1;
    throw std::runtime_error("listen failed");
  }
}

Socket Socket::connectToServer(const std::string & server, std::uint16_t port) {
  Socket s;
  s.connect(server, port);
  return s;
}

void Socket::connect(const std::string & server, std::uint16_t port) {
  addrinfo hints{}, *res, *p;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (port == 0) {
    port = 80;
  }

  std::string port_str = std::to_string(port);

  if (::getaddrinfo(server.c_str(), port_str.c_str(), &hints, &res) != 0) {
    throw std::runtime_error("getaddrinfo failed: " + server + ":" + port_str);
  }

  for (p = res; p != nullptr; p = p->ai_next) {
    int new_fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (new_fd < 0) {
      continue;
    }
    if (::connect(new_fd, p->ai_addr, p->ai_addrlen) == 0) {
      fd_ = new_fd;
      break;
    }

    ::close(new_fd);
  }

  ::freeaddrinfo(res);

  if (p == nullptr) {
    throw std::runtime_error("Could not connect to " + server + ":" + port_str);
  }
}

void Socket::sendAll(const char * data, std::size_t len) const {
  std::size_t sent = 0;
  while (sent < len) {
    ssize_t n = ::send(fd_, data + sent, len - sent, 0);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      throw std::runtime_error(std::string("send failed: ") + std::strerror(errno));
    }
    sent += static_cast<std::size_t>(n);
  }
}

int Socket::release() noexcept {
    int out = fd_;
    fd_ = -1;
    return out;
}

