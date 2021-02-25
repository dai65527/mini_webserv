# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dnakano <dnakano@student.42tokyo.jp>       +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/01/05 17:34:06 by dnakano           #+#    #+#              #
#    Updated: 2021/02/25 14:17:53 by dnakano          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CXX			:=	clang++
CPPFLAGS	:=	-Wall -Wextra -Werror

SRCS		:=	main.cpp Session.cpp Socket.cpp
OBJS		:=	$(SRCS:%.cpp=%.o)
NAME		:=	mini_webserv
OUTDIR		:=	.

.PHONY:		all
all:		$(NAME)

$(NAME):	$(OBJS)
			$(CXX) $(CPPFLAGS) $(OBJS) -o $(NAME)

.PHONY:		test
test:		$(NAME)
			$(OUTDIR)/$(NAME)

.PHONY:		clean
clean:
			rm -f $(OBJS)

.PHONY:		fclean
fclean:		clean
			rm -f $(NAME)

.PHONY:		re
re:			fclean all
