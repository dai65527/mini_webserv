/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 21:41:21 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/26 17:00:34 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Session.hpp"

#include <fcntl.h>
#include <signal.h>  // kill
#include <sys/socket.h>
#include <sys/wait.h>  // waitpid
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
** must not close fd in destructor (or fd will be closed when adding to list)
*/

Session::~Session() {}

/*
** getters
*/

int Session::getStatus() const { return status_; }
int Session::getSockFd() const { return sock_fd_; }
int Session::getFileFd() const { return file_fd_; }
int Session::getCgiInputFd() const { return cgi_input_fd_; }
int Session::getCgiOutputFd() const { return cgi_output_fd_; }

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
  /// TODO: add request perser function here
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

  n = send(sock_fd_, response_buf_.c_str(), response_buf_.length(), 0);
  if (n == -1) {
    std::cout << "[error] failed to send response" << std::endl;
    if (retry_count_ == RETRY_TIME_MAX) {
      std::cout << "[error] close connection" << std::endl;
      close(sock_fd_);
      return -1;  // return -1 if error (this session will be closed)
    }
    retry_count_++;
    return 0;
  }
  response_buf_.erase(0, n);  // erase data already sent
  if (response_buf_.empty()) {
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
  if (!request_buf_.compare(0, 3, "cgi", 0, 3)) {
    int http_status = createCgiProcess();  //
    if (http_status != HTTP_200) {
      std::cout << "[error] failed to create cgi process" << std::endl;
      response_buf_ = "cannot execute cgi";  // TODO: func create error response
      return SESSION_FOR_CLIENT_SEND;
    }
    return SESSION_FOR_CGI_WRITE;

  // create response from file
  } else if (!request_buf_.compare(0, 4, "read", 0, 4)) {
    file_fd_ = open("hello.txt", O_RDONLY);      // toriaezu
    if (file_fd_ == -1) {
      response_buf_ = "404 not found";
      return SESSION_FOR_CLIENT_SEND;
    }
    fcntl(file_fd_, F_SETFL, O_NONBLOCK);
    return SESSION_FOR_FILE_READ;

  // write to file
  } else if (!request_buf_.compare(0, 4, "write", 0, 4)) {
    file_fd_ = open("./test_req.txt", O_RDWR | O_CREAT, 0777);   // toriaezu
    if (file_fd_ == -1) {
      response_buf_ = "503 forbidden";
      return SESSION_FOR_CLIENT_SEND;
    }
    fcntl(file_fd_, F_SETFL, O_NONBLOCK);
    return SESSION_FOR_FILE_WRITE;
  }

  response_buf_ = request_buf_;  // TODO: create function to make response
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
  if (pipe(pipe_stdout) == -1) {
    std::cout << "[error] failed to create cgi process" << std::endl;
    close(pipe_stdin[0]);
    close(pipe_stdin[1]);
    return HTTP_500;
  }

  // create cgi process
  cgi_pid_ = fork();
  if (cgi_pid_ == -1) {  // close pipe if failed
    close(pipe_stdin[0]);
    close(pipe_stdin[1]);
    close(pipe_stdout[0]);
    close(pipe_stdout[1]);
    std::cout << "[error] failed to create cgi process" << std::endl;
    return HTTP_500;
  } else if (cgi_pid_ == 0) {  // cgi process (child)
    if (dup2(pipe_stdin[0], 0) == -1 || dup2(pipe_stdout[1], 1) == -1) {
      std::cerr << "[error] dup2 failed in cgi process" << std::endl;
      close(0);
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
    char* argv[] = {(char*)"/bin/cat", (char*)"-e", NULL};
    execve("/bin/cat", argv, NULL);
    exit(1);
  }

  // save piped fd and set to non blocking
  cgi_input_fd_ = pipe_stdin[1];
  cgi_output_fd_ = pipe_stdout[0];

  // set as non blocking fd
  fcntl(cgi_input_fd_, F_SETFL, O_NONBLOCK);
  fcntl(cgi_output_fd_, F_SETFL, O_NONBLOCK);

  // close fd not to use in parent process
  close(pipe_stdin[0]);
  close(pipe_stdout[1]);

  // change status to cgi write
  status_ = SESSION_FOR_CGI_WRITE;

  // return status 200 on success (but not a final status)
  return HTTP_200;
}

/*
** function: writeToCgiProcess
*/

int Session::writeToCgiProcess() {
  ssize_t n;

  // write to cgi process
  n = write(cgi_input_fd_, request_buf_.c_str(), request_buf_.length());

  // retry several times even if write failed
  if (n == -1) {
    std::cout << "[error] failed to write to CGI process" << std::endl;

    // give up if reached retry count to maximum
    if (retry_count_ == RETRY_TIME_MAX) {
      retry_count_ = 0;

      // close connection
      std::cout << "[error] close connection from CGI process" << std::endl;
      close(cgi_input_fd_);

      // expect response from cgi process
      status_ = SESSION_FOR_CGI_READ;  // to read from cgi process
      return 0;
    }

    retry_count_++;
    return 0;
  }

  // reset retry conunt on success
  retry_count_ = 0;

  // erase written data
  request_buf_.erase(0, n);

  // written all data
  if (request_buf_.empty()) {
    close(cgi_input_fd_);
    status_ = SESSION_FOR_CGI_READ;  // to read from cgi process
    return 0;
  }

  // to next read
  return 0;
}

/*
** function: readFromCgiProcess
*/

int Session::readFromCgiProcess() {
  ssize_t n;
  char read_buf[BUFFER_SIZE];

  // read from cgi process
  n = read(cgi_output_fd_, read_buf, BUFFER_SIZE);

  // retry seveal times even if read failed
  if (n == -1) {
    std::cout << "[error] failed to read from cgi process" << std::endl;
    if (retry_count_ == RETRY_TIME_MAX) {
      retry_count_ = 0;

      // close connection and make error responce
      std::cout << "[error] close connection to CGI process" << std::endl;
      close(cgi_output_fd_);
      response_buf_ = "500 internal server error";  // TODO: make response func

      // kill the process on error (if failed kill, we can do nothing...)
      if (kill(cgi_pid_, SIGKILL) == -1) {
        std::cout << "[error] failed kill cgi process" << std::endl;
      }

      // to send error response to client
      status_ = SESSION_FOR_CLIENT_SEND;
      return 0;
    }
    retry_count_++;
    return 0;
  }

  // reset retry conunt on success
  retry_count_ = 0;

  // check if pipe closed
  if (n == 0) {
    close(cgi_output_fd_);              // close pipefd
    status_ = SESSION_FOR_CLIENT_SEND;  // set for send response
    return 0;
  }

  // append data to response
  response_buf_.append(read_buf, n);

  return 0;
}

/*
** function: readFromFile
**
** read from file refered by file_fd_ and store to response
*/

int Session::readFromFile() {
  ssize_t n;
  char read_buf[BUFFER_SIZE];

  // read from file
  n = read(file_fd_, read_buf, BUFFER_SIZE);

  // retry seveal times even if read failed
  if (n == -1) {
    std::cout << "[error] failed to read from file" << std::endl;
    if (retry_count_ == RETRY_TIME_MAX) {
      retry_count_ = 0;

      // close file and make error responce
      std::cout << "[error] close file" << std::endl;
      close(cgi_output_fd_);
      response_buf_ = "500 internal server error";  // TODO: make response func

      // to send error response to client
      status_ = SESSION_FOR_CLIENT_SEND;
      return 0;
    }
    retry_count_++;
    return 0;
  }

  // reset retry conunt on success
  retry_count_ = 0;

  // check if reached eof
  if (n == 0) {
    close(cgi_output_fd_);              // close pipefd
    status_ = SESSION_FOR_CLIENT_SEND;  // set for send response
    return 0;
  }

  // append data to response
  response_buf_.append(read_buf, n);

  return 0;
}

/*
** function: writeToFile
**
** read from file refered by file_fd_ and store to response
*/

int Session::writeToFile() {
  ssize_t n;

  // write to file
  n = write(file_fd_, request_buf_.c_str(), request_buf_.length());

  // retry several times even if write failed
  if (n == -1) {
    std::cout << "[error] failed to write to file" << std::endl;

    // give up if reached retry count to maximum
    if (retry_count_ == RETRY_TIME_MAX) {
      retry_count_ = 0;

      // close connection
      std::cout << "[error] close file" << std::endl;
      close(file_fd_);

      // send response to notify request failed
      response_buf_ = "500 server error";
      status_ = SESSION_FOR_CLIENT_SEND;  // to send response to client
      return 0;
    }

    retry_count_++;
    return 0;
  }

  // reset retry conunt on success
  retry_count_ = 0;

  // erase written data
  request_buf_.erase(0, n);

  // written all data
  if (request_buf_.empty()) {
    close(file_fd_);

    // create response to notify the client
    response_buf_ = "201 created";
    status_ = SESSION_FOR_CLIENT_SEND;  // to send response to client
    return 0;
  }

  // to next read
  return 0;
}
