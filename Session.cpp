/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 21:41:21 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/25 23:15:39 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Session.hpp"

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <string>

/*
** constructor
**
** initialize fd and status
**    - status is initialized SESSION_FOR_CLIENT_RECV first
*/

Session::Session(int sock_fd) : sock_fd_(sock_fd) {
  status_ = SESSION_FOR_CLIENT_RECV;  // will receive request at first
  retry_count_ = 0;                   // init retry count
}

/*
** default constructor
**
** will be used only in list<Session>
*/

Session::Session() : sock_fd_(0), status_(SESSION_NOT_INIT) {}

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
  sock_fd_ = rhs.sock_fd_;
  status_ = rhs.status_;
  request_buf_ = rhs.request_buf_;
  response_buf_ = rhs.response_buf_;
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
int Session::getSockFd() const { return sock_fd_; }

/*
** function: recvReq
**
** receive request from client
*/

int Session::recvReq() {
  ssize_t n;
  char read_buf[BUFFER_SIZE];

  n = recv(sock_fd_, read_buf, BUFFER_SIZE, 0);
  if (n == -1) {
    if (retry_count_ == RETRY_TIME_MAX) {
      close(sock_fd_);
      return -1;  // return -1 if error (this session will be closed)
    }
    retry_count_++;
    return 0;
  }
  request_buf_.append(read_buf, n);
  if (n == 1 /* this will be resulted from content of request */) {
    retry_count_ = 0;
    status_ = createResponse();
    return 1;
  }
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

  n = send(sock_fd_, request_buf_.c_str(), request_buf_.length(), 0);
  if (n == -1) {
    if (retry_count_ == RETRY_TIME_MAX) {
      close(sock_fd_);
      return -1;  // return -1 if error (this session will be closed)
    }
    retry_count_++;
    return 0;
  }
  request_buf_.erase(0, n);  // erase data already sent
  if (request_buf_.empty()) {
    close(sock_fd_);
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
  // create cgi process if requested
  if (true /* will add function check if cgi is needed */) {
    int http_status = createCgiProcess();   // 
    if (http_status != HTTP_200) {
      std::cout << "[error] failed to create cgi process" << std::endl;
      response_buf_ = "cannot execute cgi"; // TODO: func create error response
      return SESSION_FOR_CLIENT_SEND;
    }
    return SESSION_FOR_CGI_WRITE;
  }

  // from file (to be implemented)
  response_buf_ = request_buf_;   // TODO: create function to make response
  return SESSION_FOR_CLIENT_SEND;
}

/*
** function: createCgiProcess
**
** create proccess to execute CGI process
**    - create environment variables and input string for cgi process (TODO!!)
**    - create piped fds connected to stdin and stdout of CGI process
**    - create child process for cgi and execute cgi program
*/

int Session::createCgiProcess() {
  // create a pipe connect to stdin of cgi process
  int pipe_stdin[2];
  if (pipe(pipe_stdin) == -1) {
    std::cout << "[error] failed to create cgi process" << std::endl;
    return HTTP_500;
  }

  // create a pipe connect to stdout of cgi process
  int pipe_stdout[2];
  if (pipe(pipe_stdin) == -1) {
    std::cout << "[error] failed to create cgi process" << std::endl;
    close(pipe_stdin[0]);
    close(pipe_stdin[1]);
    return HTTP_500;
  }

  // create cgi process
  cgi_pid_ = fork();
  if (cgi_pid_ == -1) {         // close pipe if failed
    close(pipe_stdin[0]);
    close(pipe_stdin[1]);
    close(pipe_stdout[0]);
    close(pipe_stdout[1]);
    std::cout << "[error] failed to create cgi process" << std::endl;
    return HTTP_500;
  } else if (cgi_pid_ == 0) {   // cgi process (child)
    // connect stdin and stdout to pipe
    if (dup2(0, pipe_stdin[0]) == -1 || dup2(1, pipe_stdout[1]) == -1) {
      close(pipe_stdin[0]);
      close(pipe_stdin[1]);
      close(pipe_stdout[0]);
      close(pipe_stdout[1]);
      exit(1);
    }

    // close no longer needed pipe fd
    close(pipe_stdin[0]);
    close(pipe_stdin[1]);
    close(pipe_stdout[0]);
    close(pipe_stdout[1]);

    // excecute cgi process (TODO: implement iroiro)
    char *argv[] = { "/bin/cat", "-e", NULL };
    execve("/bin/cat", argv, NULL);
    exit(1);
  }

  // save piped fd and set to non blocking
  cgi_input_fd_ = pipe_stdin[1];
  cgi_output_fd_ = pipe_stdout[0];
  close(pipe_stdin[0]);
  close(pipe_stdout[1]);
  fcntl(cgi_input_fd_, F_SETFL, O_NONBLOCK);
  fcntl(cgi_output_fd_, F_SETFL, O_NONBLOCK);

  // return status 200 on success
  return HTTP_200;
}
