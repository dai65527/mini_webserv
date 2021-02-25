/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 21:41:21 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/25 12:26:39 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Session.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <string>

/*
** constructor
**
** initialize fd and status
**    - status is initialized SESSION_FOR_CLIENT_RECV first
*/

Session::Session(int sock_fd) : fd_(sock_fd) {
  status_ = SESSION_FOR_CLIENT_RECV;  // will receive request at first
  retry_count_ = 0;                   // init retry count
}

/*
** default constructor
**
** will be used only in list<Session>
*/

Session::Session() : fd_(0), status_(SESSION_NOT_INIT) {}

/*
** copy constructor
**
** will be used only in list<Session>
*/

Session::Session(const Session& ref) { *this = ref; }

/*
** assignation operator overload
**
** will be used only in list<Session>
*/

Session& Session::operator=(const Session& rhs) {
  if (this == &rhs) {
    return *this;
  }
  fd_ = rhs.fd_;
  status_ = rhs.status_;
  buf_ = rhs.buf_;
  retry_count_ = rhs.retry_count_;
  return *this;
}

/*
** destctor
**
** do nothing
** must not close fd in destructor (fd will be closed when adding to list)
*/

Session::~Session() {}

/*
** getters
*/

int Session::getStatus() const { return status_; }
int Session::getFd() const { return fd_; }

/*
** function: recvReq
**
** receive request from client
*/

int Session::recvReq() {
  ssize_t n;
  char read_buf[BUFFER_SIZE];

  n = recv(fd_, read_buf, BUFFER_SIZE, 0);
  if (n == -1) {
    if (retry_count_ == RETRY_TIME_MAX) {
      close(fd_);
      return -1;  // return -1 if error (this session will be closed)
    }
    retry_count_++;
    return 0;
  } else if (n == 1) {    // when got EOF
    status_ = SESSION_FOR_CLIENT_SEND;
    retry_count_ = 0;
    createResponse();
    return 1;
  }
  buf_.append(read_buf, n);
  retry_count_ = 0;
  return 0;
}

/*
** function: recvReq
**
** send response to client
*/

int Session::sendRes() {
  ssize_t n;

  n = send(fd_, buf_.c_str(), buf_.length(), 0);
  if (n == -1) {
    if (retry_count_ == RETRY_TIME_MAX) {
      close(fd_);
      return -1;  // return -1 if error (this session will be closed)
    }
    retry_count_++;
    return 0;
  }
  buf_.erase(0, n);  // erase data already sent
  if (buf_.empty()) {
    close(fd_);
    return 1;  // return 1 if all data sent (this session will be closed)
  }
  retry_count_ = 0;  // reset retry_count if success
  return 0;
}

/*
** function: recvReq
**
** send response to client
*/

int Session::createResponse() {
  // 何もせず返す（とりあえずオウム返し）
  return 1;
}
