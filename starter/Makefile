################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules.                              #
#                                                                              #
# Authors: Hong Jai Cho <hongjaic@andrew.cmu.edu>,                             #
#          Raul Gonzalez <rggonzal@andrew.cmu.edu>                             #
#                                                                              #
################################################################################

default: testfile 
#routing_daemon

testfile:
	@gcc test.c hashset.c -o testfile -Wall -Werror

#routing_daemon:
#	@gcc  -Wall -Werror

clean:
	@rm testfile routing_daemon
