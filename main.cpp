/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 15:18:18 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/24 18:27:27 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/select.h>

#include <exception>
#include <iostream>
#include <list>
#include <string>

#include "Session.hpp"
#include "Socket.hpp"
#include "config.hpp"

void server() {
  int n_fd;           // number of fds ...
                      // waiting to get ready (before select)
                      // or ready to read/write (after select)
  fd_set rfd;         // set of read fd
  fd_set wfd;         // set of write fd
  Socket sock;        // socket for listing
  std::list<Session> sessions;

  // initialize timeout of select
  struct timeval tv_timeout;  // time to timeout
  tv_timeout.tv_sec = SELECT_TIMEOUT_MS / 1000;
  tv_timeout.tv_usec = (SELECT_TIMEOUT_MS * 1000) % 1000000;

  // initialize socket
  sock.init(DEFAULT_PORT);

  // main loop
  while (1) {
    // initialize fd sets
    n_fd = 0;
    FD_ZERO(&rfd);
    FD_ZERO(&wfd);

    // set listing socket fd
    FD_SET(sock.getFd(), &rfd);

    // set sessions fd
    for (std::list<Session>::iterator itr = sessions.begin();
         itr != sessions.end(); ++itr) {
      if (itr->status() == SESSION_FOR_CLIENT_RECV) {
        FD_SET(itr->getSockFd(), &rfd);
        n_fd++;
      } else if (itr->status() == SESSION_FOR_CLIENT_SEND) {
        FD_SET(itr->getSockFd(), &wfd);
        n_fd++;
      }
    }

    // wait for fds getting ready
    n_fd = select(n_fd, &rfd, &wfd, NULL, &tv_timeout);
    if (n_fd == -1) {
      std::cout << "[error]: select" << std::endl;
    } else if (n_fd == 0) {
      continue;
    }

    // check each session and recv/send if ready
    for (std::list<Session>::iterator itr = sessions.begin();
         itr != sessions.end(), n_fd > 0; ++itr) {
      if (FD_ISSET(itr->getSockFd(), &rfd)) {
        itr->recvReq();
        n_fd--;
      } else if (FD_ISSET(itr->getSockFd(), &rfd)) {
        itr->sendRes();
        n_fd--;
      }
    }

    // accept connection and add to sessions list
    if (FD_ISSET(sock.getFd(), &rfd)) {
      int accepted_fd;
      // accept all incoming connections (rest of n_fd)
      while (n_fd-- > 0) {
        accepted_fd = sock.acceptRequest();
        if (accepted_fd >= 0) {
          sessions.push_back(Session(accepted_fd));
        }
      }
    }
  }
}

int main(void) {
  try {
    server();
  } catch (std::exception e) {
    std::cout << e.what() << std::endl;
  }
  return 1;
}
