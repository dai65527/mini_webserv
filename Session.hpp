/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 16:26:56 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/24 18:40:07 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_HPP
#define SESSION_HPP

#define SESSION_NOT_READY       0x0000
#define SESSION_FOR_CLIENT_RECV 0x0001
#define SESSION_FOR_CLIENT_SEND 0x0002
#define SESSION_FOR_CGI_RECV    0x0011 // to be implemented
#define SESSION_FOR_CGI_SEND    0x0012 // to be implemented

class Session
{
private:
  int sock_fd_;
  int status_;

  // do not allow copy and assignation
  Session();
  Session(const Session& ref);
  Session& operator=(const Session& ref);

public:
  Session(int socket_fd);
  ~Session();

  int status() const;
  int getSockFd() const;

  void recvReq();
  void sendRes();
};

#endif /* SESSION_HPP */
