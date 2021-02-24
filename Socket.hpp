/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 15:38:38 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/24 20:57:49 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <netinet/in.h>  // sockaddr_in

#include "config.hpp"

class Socket {
 private:
  int fd_;                      // socket's fd
  int port_;                    // port number
  struct sockaddr_in addr_in_;  // address of socket (in ipv4)
  socklen_t addrlen_;           // address byte length (used in accept)

  // do not allow copy and assignation
  Socket(const Socket& ref);
  Socket& operator=(const Socket& ref);

 public:
  Socket();
  ~Socket();

  // getter
  int getFd() const;

  // function to init a socket
  void init(int port);

  // returns a file discripor of accepted socket (or -1 if error)
  int acceptRequest();
};

#endif /* SOCKET_HPP */
