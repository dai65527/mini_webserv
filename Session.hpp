/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 16:26:56 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/27 14:12:03 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_HPP
#define SESSION_HPP

#include <sys/types.h>

#include <string>

#include "config.hpp"

// #define SESSION_NOT_INIT 0x0000
// #define SESSION_FOR_CLIENT_RECV 0x0001
// #define SESSION_FOR_CLIENT_SEND 0x0002
// #define SESSION_FOR_CGI_WRITE 0x0011
// #define SESSION_FOR_CGI_READ 0x0012
// #define SESSION_FOR_FILE_READ 0x0021
// #define SESSION_FOR_FILE_WRITE 0x0022

#define HTTP_200 200  // 200 OK
#define HTTP_403 403  // 403 Forbidden
#define HTTP_404 404  // 404 Not Found
#define HTTP_418 418  // 418 I'm a teapot
#define HTTP_500 500  // 500 Internal Server Error
#define HTTP_501 501  // 501 Not Implemented
#define HTTP_502 502  // 502 Bad Gateway

// sessionStatus
enum SessionStatus {
  SESSION_NOT_INIT,
  SESSION_FOR_CLIENT_RECV,
  SESSION_FOR_CLIENT_SEND,
  SESSION_FOR_CGI_WRITE,
  SESSION_FOR_CGI_READ,
  SESSION_FOR_FILE_READ,
  SESSION_FOR_FILE_WRITE
};

class Session {
 private:
  SessionStatus status_;      // status of session (defined by SESSION_XXX)
  int sock_fd_;               // fd of socket to client
  int cgi_input_fd_;          // cgi_fd_[0] will connected to STDIN of cgi
  int cgi_output_fd_;         // cgi_fd_[1] will connected to STDOUT of cgi
  int file_fd_;               // fd of file to read/write
  pid_t cgi_pid_;             // pid of cgi process
  std::string request_buf_;   // to store request
  std::string response_buf_;  // to store response
  std::string filename;       // to store filename to read/write
  int retry_count_;           // use to count failure

 public:
  Session();
  Session(int sock_fd);
  Session& operator=(const Session& ref);
  Session(const Session& ref);
  ~Session();

  // getters
  SessionStatus getStatus() const;
  int getSockFd() const;
  int getFileFd() const;
  int getCgiInputFd() const;
  int getCgiOutputFd() const;

  int recvReq();
  int sendRes();
  SessionStatus createResponse();
  int createCgiProcess();
  int writeToCgiProcess();
  int readFromCgiProcess();
  int readFromFile();
  int writeToFile();
};

#endif /* SESSION_HPP */
