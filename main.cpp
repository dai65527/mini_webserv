/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 15:18:18 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/26 16:49:44 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/select.h>
#include <signal.h>

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

  // ignore sigchld signal
  signal(SIGCHLD, SIG_IGN);

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
        FD_SET(itr->getSockFd(), &rfd);
        max_fd = std::max(max_fd, itr->getSockFd());
      } else if (itr->getStatus() == SESSION_FOR_FILE_READ) {
        FD_SET(itr->getFileFd(), &rfd);
        max_fd = std::max(max_fd, itr->getFileFd());
      } else if (itr->getStatus() == SESSION_FOR_FILE_WRITE) {
        FD_SET(itr->getFileFd(), &wfd);
        max_fd = std::max(max_fd, itr->getFileFd());
      } else if (itr->getStatus() == SESSION_FOR_CGI_WRITE) {
        FD_SET(itr->getCgiInputFd(), &wfd);
        max_fd = std::max(max_fd, itr->getCgiInputFd());
      } else if (itr->getStatus() == SESSION_FOR_CGI_READ) {
        FD_SET(itr->getCgiOutputFd(), &rfd);
        max_fd = std::max(max_fd, itr->getCgiOutputFd());
      } else if (itr->getStatus() == SESSION_FOR_CLIENT_SEND) {
        FD_SET(itr->getSockFd(), &wfd);
        max_fd = std::max(max_fd, itr->getSockFd());
      }
    }

    // wait for fds getting ready
    n_fd = select(max_fd + 1, &rfd, &wfd, NULL, &tv_timeout);
    if (n_fd == -1) {
      std::cout << "[error]: select" << std::endl;
    } else if (n_fd == 0) {
      continue;
    }

    // check each session if it is ready to recv/send
    for (std::list<Session>::iterator itr = sessions.begin();
         itr != sessions.end() && n_fd > 0;) {
      if (FD_ISSET(itr->getSockFd(), &rfd)) {
        if (itr->recvReq() == -1) {
          itr = sessions.erase(itr);    // delete session if failed to recv
        } else {
          std::cout << "[webserv] received request data" << std::endl;
          ++itr;
        }
        n_fd--;
      } else if (FD_ISSET(itr->getFileFd(), &rfd)) {
        if (itr->readFromFile() == -1) {
          itr = sessions.erase(itr);    // delete session if failed or ended
        } else {
          std::cout << "[webserv] read data from file" << std::endl;
          ++itr;
        }
        n_fd--;
      } else if (FD_ISSET(itr->getFileFd(), &wfd)) {
        if (itr->writeToFile() == -1) {
          itr = sessions.erase(itr);    // delete session if failed or ended
        } else {
          std::cout << "[webserv] write data to file" << std::endl;
          ++itr;
        }
        n_fd--;
      } else if (FD_ISSET(itr->getCgiInputFd(), &wfd)) {
        if (itr->writeToCgiProcess() == -1) {
          itr = sessions.erase(itr);    // delete session if failed
        } else {
          std::cout << "[webserv] wrote data to cgi" << std::endl;
          ++itr;
        }
        n_fd--;
      } else if (FD_ISSET(itr->getCgiOutputFd(), &rfd)) {
        if (itr->readFromCgiProcess() == -1) {
          itr = sessions.erase(itr);    // delete session if failed
        } else {
          std::cout << "[webserv] read data from cgi" << std::endl;
          ++itr;
        }
        n_fd--;
      } else if (FD_ISSET(itr->getSockFd(), &wfd)) {
        if (itr->sendRes() != 0) {
          std::cout << "[webserv] sent response data" << std::endl;
          itr = sessions.erase(itr);    // delete session if failed or ended
        } else {
          ++itr;
        }
        n_fd--;
      } else {
        ++itr;
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
