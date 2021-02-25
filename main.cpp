/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 15:18:18 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/25 12:28:31 by dnakano          ###   ########.fr       */
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
  int n_fd;           // number of fds ready to read/write (value from select)
  int max_fd;         // maximum nubmer of fds (to pass select())
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
  std::cout << "socket initialized" << std::endl;

  // main loop
  while (1) {
    // initialize fd sets
    n_fd = 0;
    FD_ZERO(&rfd);
    FD_ZERO(&wfd);

    // set listing socket fd
    FD_SET(sock.getFd(), &rfd);
    max_fd = sock.getFd();

    // set sessions fd
    for (std::list<Session>::iterator itr = sessions.begin();
         itr != sessions.end(); ++itr) {
      if (itr->getStatus() == SESSION_FOR_CLIENT_RECV) {
        FD_SET(itr->getFd(), &rfd);
        max_fd = std::max(max_fd, itr->getFd());
      } else if (itr->getStatus() == SESSION_FOR_CLIENT_SEND) {
        FD_SET(itr->getFd(), &wfd);
        max_fd = std::max(max_fd, itr->getFd());
      }
    }

    // wait for fds getting ready
    std::cout << "listening..." << std::endl;
    n_fd = select(max_fd + 1, &rfd, &wfd, NULL, &tv_timeout);
    if (n_fd == -1) {
      std::cout << "[error]: select" << std::endl;
    } else if (n_fd == 0) {
      continue;
    }

    // check each session and recv/send if ready
    for (std::list<Session>::iterator itr = sessions.begin();
         itr != sessions.end() && n_fd > 0;) {
      if (FD_ISSET(itr->getFd(), &rfd)) {
        if (itr->recvReq() == -1) {
          itr = sessions.erase(itr);    // delete session if failed to recv
        } else {
          ++itr;
        }
        n_fd--;
      } else if (FD_ISSET(itr->getFd(), &wfd)) {
        if (itr->sendRes() != -1) {
          itr = sessions.erase(itr);    // delete session if failed or ended
        } else {
          ++itr;
        }
        n_fd--;
      }
    }

    // accept new connection and add to sessions list
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
