/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/24 15:31:12 by dnakano           #+#    #+#             */
/*   Updated: 2021/02/25 08:01:30 by dnakano          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

/*
** header file to write a config
*/

// default port number
#define DEFAULT_PORT 8088

// time to timeout of select (in msec)
#define SELECT_TIMEOUT_MS 2500

// que length of tcp socket
#define SOCKET_QUE_LEN 128

// buffer size
#define BUFFER_SIZE 8096

// retry max time to retry to recv/send
#define RETRY_TIME_MAX 10

#endif /* CONFIG_HPP */
