################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules.                              #
#                                                                              #
# Authors: Hong Jai Cho <hongjaic@andrew.cmu.edu>,                             #
#          Raul Gonzalez <rggonzal@andrew.cmu.edu>                             #
#                                                                              #
################################################################################

default: rd
#testfile

#testfile:
#	@gcc test.c hashset.c -o testfile -Wall -Werror

rd: engine_actions.c hashset.c file_loader.c flooding_engine.c flask_engine.c tcp_connection.c
	@gcc rd.c engine_actions.c hashset.c file_loader.c flooding_engine.c flask_engine.c tcp_connection.c -o rd -g -Wall -Werror

clean:
	@rm rd
#testfile
