/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 16:26:56 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/25 08:58:25 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>

#include "config.hpp"

#define SESSION_NOT_INIT 0x0000
#define SESSION_FOR_CLIENT_RECV 0x0001
#define SESSION_FOR_CLIENT_SEND 0x0002
#define SESSION_FOR_CGI_RECV 0x0011  // to be implemented
#define SESSION_FOR_CGI_SEND 0x0012  // to be implemented
#define SESSION_PROCESSING 0x0100

class Session {
 private:
  int fd_;
  int status_;
  std::string buf_;
  int retry_count_;

 public:
  Session();
  Session(int sock_fd);
  Session& operator=(const Session& ref);
  Session(const Session& ref);
  ~Session();

  // getters
  int getStatus() const;
  int getFd() const;

  int recvReq();
  int sendRes();
  int createResponse();
};

#endif /* SESSION_HPP */
