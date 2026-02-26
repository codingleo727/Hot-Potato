#pragma once
#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <cstddef>
#include <cstdint>
#include <string>

class Socket {
public:
  Socket() noexcept;
  explicit Socket(int fd) noexcept;
  ~Socket();

  Socket(const Socket &) = delete;
  Socket & operator=(const Socket &) = delete;

  Socket(Socket && other) noexcept;
  Socket & operator=(Socket && other) noexcept;

  /**
   * Get the underlying file descriptor.
   * @return the file descriptor, or -1 if the Socket is not managing a valid
   */
  int get_fd() const noexcept;
  /**
   * Check if the socket is valid (i.e., has a valid file descriptor).
   * @return true if the socket is valid, false otherwise
   */
  bool valid() const noexcept;

  /**
   * Close the socket if it's valid. After calling this function, the Socket object will no longer manage a valid file descriptor and will not close it on destruction.
   */
  void close() noexcept;

  /**
    * Receive data from the socket, blocking until at least 1 byte is received.
    * @param buf buffer to receive data into
    * @param len length of the buffer
    * @return number of bytes received (0 means peer closed)
    */
  std::size_t recvSome(char * buf, std::size_t len) const;

  /**
   * Receive exactly len bytes of data from the socket, blocking until all data is received. If the peer closes the connection before all data is received, a runtime_error exception will be thrown.
   * @param buf buffer to receive data into
   * @param len length of the buffer
   * @throws std::runtime_error if the peer closes the connection before all data is received
   */
  void recvAll(char * buf, std::size_t len) const;

  /**
    * Create a listening socket on the given port.
    * @param port the port number to listen on
    * @return a Socket object representing the listening socket
    */
  static Socket createListeningSocket(std::uint16_t port);

  /**
    * Connect to a server.
    * @param origin the hostname of the other server
    * @param port the port number of the other server
    * @return a Socket object representing the connection to the other server
    */
  static Socket connectToServer(const std::string & server, std::uint16_t port);
  
  /**
    * Send all data in the buffer, blocking until all data is sent.
    * @param data the buffer containing the data to send
    * @param len the length of the data to send
    */
  void sendAll(const char * data, std::size_t len) const;
  
  /**
    * Release the underlying file descriptor, returning it. After calling this function, the Socket object will no longer manage the file descriptor and will not close it on destruction.
    * @return the released file descriptor, or -1 if the Socket was not managing a valid file descriptor
    */
  int release() noexcept;
private:
  int fd_;
  /**
    * Listen on the given port.
    * @param port the port number to listen on
    */
  void listen(std::uint16_t port);

  /**
    * Connect to a server.
    * @param server the hostname of the other server
    * @param port the port number of the other server
    */
  void connect(const std::string & server, std::uint16_t port);
};
#endif