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

rd: rd.c
	@gcc rd.c hashset.c flooding_engine.c -o rd -g -Wall -Werror

clean:
	@rm rd
#testfile
